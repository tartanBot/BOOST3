#!/usr/bin/python
import requests
import json
import rospy
from std_msgs.msg import String
import std_msgs.msg
import nav_msgs.msg
from nav_msgs.msg import Path
from geometry_msgs.msg import Pose
import pymap3d as pm 
import geometry_msgs.msg
from sensor_msgs.msg import NavSatFix

pub = rospy.Publisher('chatter', Path, queue_size = 10)
SEQ = 0
def talker(data):
    global SEQ
    if data.status >10 or True:
        origin = str(data.latitude)+'%2C'+str(data.longitude)
        # origin = 'University+Center,+Forbes+Avenue,+Pittsburgh,+PA' # Enter Place name as shown or Lat,Lon as "Lat%2CLong"
        destination = 'Posner+Hall,+Pittsburgh,+PA+15213' # Enter Place name as shown or Lat,Lon as "Lat%2CLong"
        key = 'AIzaSyAfG60NGmeaR5TaOdwliyG18ux_VUj68Rk' # Dont change this Key. Unless you apply for a new key yourself.
        url = 'https://maps.googleapis.com/maps/api/directions/json?&mode=walking&origin='+origin+'&destination='+destination+'&key='+key    
        response = requests.get(url)    
        map_json = response.json()
        print("START_LOCATION = " + json.dumps(map_json["routes"][0]["legs"][0]["steps"][0]["start_location"], indent=4, sort_keys=True))
        # print(json.dumps(map_json,indent = 4, sort_keys = True))
        lat_arr = [] 
        long_arr = []
        start_lat = float(map_json["routes"][0]["legs"][0]["steps"][0]["start_location"]['lat'])
        start_lon = float(map_json["routes"][0]["legs"][0]["steps"][0]["start_location"]['lng'])
        start_alt = data.altitude
        lat_arr.append(start_lat)
        long_arr.append(start_lon)
        for step in range(len(map_json["routes"][0]["legs"][0]["steps"])):
            lat_arr.append(float(map_json["routes"][0]["legs"][0]["steps"][step]["end_location"]['lat']))
            long_arr.append(float(map_json["routes"][0]["legs"][0]["steps"][step]["end_location"]['lng']))
            # print(map_json["routes"][0]["legs"][0]["steps"][step]["end_location"]['lat'])#, indent=4, sort_keys=True))
            # print(json.dumps(map_json["routes"][0]["legs"][0]["steps"][step]["end_location"], indent=4, sort_keys=True))

        print("Latitude Array = ",lat_arr)
        print("Longitude Array = ", long_arr)
        # rospy.init_node('talker', anonymous=True)
     
        rate = rospy.Rate(1) # 10hz
        # while not rospy.is_shutdown():
        # # 
        # print(geometry_msgs.msg.PoseStamped())
        path = nav_msgs.msg.Path()
        path.poses = []
        for i in range(len(lat_arr)):
            a = geometry_msgs.msg.PoseStamped()
            # h = std_msgs.msg.Header()
            # h.stamp = rospy.Time.now()
            # SEQ += 1
            # h.seq = SEQ
            # h.frame_id = str(SEQ)
            # a.header = h            
            pose = Pose()
            v,f,t = pm.geodetic2enu(lat_arr[i],long_arr[i],0,start_lat,start_lon,0)
            pose.position.x  = v
            pose.position.y  = f
            pose.position.z  = t
            pose.orientation.x =  0.0
            pose.orientation.y =  1.0
            pose.orientation.z =  2.0
            pose.orientation.w =  3.0
     
            # path.poses.append(pose)
            a.pose =pose
            # print(a.header)
            path.poses.append(a)



        pub.publish(path)
        rospy.loginfo(path)        

        rate.sleep()

def planner():
    rospy.init_node('planner', anonymous=True)
    rospy.Subscriber('chatter1', NavSatFix, talker)
    rospy.spin()
if __name__ == '__main__':
    try:
        planner()
    except rospy.ROSInterruptException:
        pass
