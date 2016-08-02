#include "ros/ros.h"
#include "std_msgs/Float32.h"
#include <std_srvs/Empty.h>

#include <geometry_msgs/Twist.h>
#include <sensor_msgs/Imu.h>
#include <nav_msgs/Odometry.h>

#include "tf/transform_broadcaster.h"

#include <sstream>
#include <string>
#include <vector>
#include <math.h>

/* TODO 
		- USE TWIST TO ENABLE LARGE CHANGES OF CALCULATED HEAD (BYPASSING LOW-PASS FILTER), STORE Z-ORIENTATION TWIST VALUE IN QUEUE NODE
		- WRITE LOW PASS FILTER FOR CHANGES IN COURSE HEADING, STORE PREVIOUS HEADING SOMEWHERE
*/

const double pi = 4*atan(1);
const double toRadians = pi / 180.0;
double prevX = 0.0;
double prevY = 0.0;
double currX = 0.0;
double currY = 0.0;
double prevTime = 0.0;
double currTime = 0.0;

int sample_size = 100;
double pub_rate = 20;

double forward_velocity = 0.1;

typedef struct list_node list;
struct list_node {
    double x;
    double y;
    double time;
    list *next;
};

typedef struct queue_header queue;
struct queue_header {
    list *front;
    list *back;
    int size;
};

void deq(queue *Q) {
// remove the first node of the queue (linked list)
    list *front_ptr = Q->front;
    if (Q->size == 1) Q->front == NULL;
    else Q->front = Q->front->next;
    Q->size--;
    delete front_ptr;
    return;
}

void enq(queue *Q, double x, double y, double time) {
// add a node to the end of the queue (linked list)
    list *l = new list;
    l->next = NULL;
    l->x = x;
    l->y = y;
    l->time = time;
    if (Q->size == 0) Q->front = l;
    else Q->back->next = l;
    Q->back = l;
    Q->size++;
    return;
}

double bestFit(queue *Q) {
    bool posX;
    bool posY; 
    double sumX = 0.0; //sum of all x points
    double sumY = 0.0; //sum of all y points
    double sumXY = 0.0; //sum of the product of each x and y pairing
    double sumXX = 0.0; //sum of the x points squared
    double n = Q->size;
    list *cur = Q->front;
    for (int i = 0; i < n; i++) {
        sumX += cur->x;
        sumY += cur->y;
        sumXY += cur->x * cur->y;
        sumXX += cur->x * cur->x;
        cur = cur->next;
    }
    list* front = Q->front;
    list* back = Q->back;
    // formula for the slope of best fit line
    double slope = (n*sumXY - sumX*sumY)/(n*sumXX - sumX*sumX); 
    double heading = atan(slope);
    
    if (fabs(slope) < 1) { // moving more in x direction
        if ((back->x - front->x) > 0) posX = true; // moving in positive x direction
        else posX = false; // moving in negative x direction
    	//adjust heading angle
	if ((posX == true) && (heading < 0)) heading = heading + 2*pi; 
        else if (posX == false) heading = heading + pi;   
    }
    else { // moving more in y direction
        if ((back->y - front->y) > 0) posY = true; // moving in positive y direction
        else posY = false; // moving in negative y direction
        //adjust heading angle 
        if ((posY == true) && (heading < 0)) heading = heading + pi;
        else if ((posY == false) && (heading < 0)) heading = heading + 2*pi;
	else if ((posY == false) && (heading >= 0)) heading = heading + pi;  
    }
        
    return heading;
}  

// TODO: ADD ALTITUDE TO GET 3D POSE ESTIMATE

void gpsHeadingCallback(const nav_msgs::Odometry::ConstPtr& utm_msg)
{
	ROS_DEBUG_STREAM("Got into callback!");
	
	// Add x and y to respective arrays, along with time, check for GBAS and fix

	prevX = currX;
	prevY = currY;
	prevTime = currTime;

	currX = utm_msg->pose.pose.position.x;
	currY = utm_msg->pose.pose.position.y;
	currTime = utm_msg->header.stamp.sec + (1e-9 * utm_msg->header.stamp.nsec);
}

bool driveForward( std_srvs::Empty::Request &req, std_srvs::Empty::Response &resp)
{

	ros::NodeHandle nh;

  	ros::Subscriber sub_joint_command = nh.subscribe("odometry/utm", 1, gpsHeadingCallback);

	// Initialize Imu publisher, which will hold the heading information calculated in the callBack
	ros::Publisher imu_pub = nh.advertise<sensor_msgs::Imu>("gps/imu/data", 1, false);

	// Start moving the robot at a constant velocity with a twist of 0.
	ros::Publisher twist_pub = nh.advertise<geometry_msgs::Twist>("nav_vel_heading", 1);
	// Populate Twist message
	geometry_msgs::Twist twist_msg;
	twist_msg.linear.x =  forward_velocity; // Fill with constant linear velocity
	twist_msg.angular.z = 0; // Fill with angular velocity		

	// Sleep to allow time for fix to be updated, HACK- FIX LATER
	sleep(1);
	ros::spinOnce();
	sleep(0.5);
	ros::spinOnce();  // Fill in currLat

	double xCheck = currX;
	double yCheck = currY;
	double fit_heading = 0;	
	
	// Create Queue header for position data
	queue *posQ = new queue;
	posQ->size = 0;
	posQ->front = NULL;
	posQ->back = NULL;

	double start_time = ros::Time::now().toSec();
	double max_time = 3 * sample_size / pub_rate;

	// Collect data points until queue is full
	ros::Rate r(pub_rate);
	while ((posQ->size < sample_size) && ((ros::Time::now().toSec()) - start_time < max_time))
	{
		// Publish twist message
		twist_pub.publish(twist_msg);

		ROS_INFO_STREAM("Time running is: " << (ros::Time::now().toSec() - start_time));
		ROS_INFO_STREAM("Max time to run is: " << max_time);
		// Check that positions were updated
		if (xCheck == currX || yCheck == currY)
		{
			ROS_DEBUG_STREAM(" X/Y have not been updated!");
			r.sleep();
			ros::spinOnce();
			continue;
		}
		double xCheck = currX;
		double yCheck = currY;

		// Calculate distance between two coordinates
		double dx = currX-prevX;
		double dy = currY-prevY;
		double dist = pow(dx*dx + dy*dy, 0.5);  // output in meters
		ROS_DEBUG("dist is: %05.16f", dist);

    	// Enqueue new position data onto the position queue
    	enq(posQ, currX, currY, currTime);
    	if (posQ->size > sample_size) deq(posQ);
        
		ROS_DEBUG("Previous Location is: x: %05.10f, y: %05.10f", prevX, prevY);
		ROS_DEBUG("Current Location is:  x: %05.10f, y: %05.10f", currX, prevY);
		
		r.sleep();
		ros::spinOnce();
	}

	// Populate and publish Imu message, stop robot
	if ((posQ->size == sample_size) && ((start_time - ros::Time::now().toSec()) < max_time))
	{
    	// Calculate Average Heading from position Queue (linear regression)
		fit_heading = bestFit(posQ);

		// Populate and publish message
		sensor_msgs::Imu imu_msg;

		imu_msg.header.frame_id = "GPS_link";
		imu_msg.header.stamp = ros::Time::now();

		geometry_msgs::Quaternion pose_quat = tf::createQuaternionMsgFromYaw(fit_heading);

		imu_msg.orientation = pose_quat;

		imu_msg.orientation_covariance[0] = 1e6;
		imu_msg.orientation_covariance[4] = 1e6;
		imu_msg.orientation_covariance[8] = 1e-2;

		imu_pub.publish(imu_msg);

		ROS_INFO_STREAM("Publishing heading! Heading is " << fit_heading);

		// Populate Twist message to stop robot
		twist_msg.linear.x =  0;
		twist_msg.angular.z = 0;	
			
		// Publish Twist message
		ROS_INFO_STREAM("Stopping robot!");
		twist_pub.publish(twist_msg);	

	    // Free memory from the position queue
	    list *cur = posQ->front;
	    for (int i = 0; i < posQ->size; i++) 
	    {
	        list *temp = cur;
	        cur = cur->next;
	        delete temp;
	    }
	    delete posQ;

		return true;
	}


	ROS_WARN_STREAM("Heading not published, check fix!");

	// Populate Twist message to stop robot
	twist_msg.linear.x =  0;
	twist_msg.angular.z = 0;	
		
	// Publish Twist message
	ROS_INFO_STREAM("Stopping robot!");
	double count = 0;
	while (count < 1000)
	{
		twist_pub.publish(twist_msg);	
		count += 1;
	}

    // Free memory from the position queue
    list *cur = posQ->front;
    for (int i = 0; i < posQ->size; i++) 
    {
        list *temp = cur;
        cur = cur->next;
        delete temp;
    }
    delete posQ;

	return false;
}

int main(int argc, char **argv)
{
	ros::init(argc, argv, "boost_avg_heading_from_gps_service_publisher");

	ros::NodeHandle n;

	// Register service with the master
	ros::ServiceServer server = n.advertiseService("acquire_Heading", &driveForward);

	ros::Rate rate(4);
	while (ros::ok())
	{
		ros::spinOnce();
		rate.sleep();
	}

}