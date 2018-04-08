import requests
import json

origin = 'University+Center,+Forbes+Avenue,+Pittsburgh,+PA' # Enter Place name as shown or Lat,Lon as "Lat%2CLong"
destination = 'Posner+Hall,+Pittsburgh,+PA+15213' # Enter Place name as shown or Lat,Lon as "Lat%2CLong"
key = 'AIzaSyAfG60NGmeaR5TaOdwliyG18ux_VUj68Rk' # Dont change this Key. Unless you apply for a new key yourself.
url = 'https://maps.googleapis.com/maps/api/directions/json?&mode=walking&origin='+origin+'&destination='+destination+'&key='+key    
response = requests.get(url)    
map_json = response.json()
# print(len(map_json["routes"][0]["legs"][0]["steps"])) # No of waypoints we will get. 
# print(json.dumps(map_json["routes"][0]["legs"][0]["steps"], indent=4, sort_keys=True)) 
print("START_LOCATION = " + json.dumps(map_json["routes"][0]["legs"][0]["steps"][0]["start_location"], indent=4, sort_keys=True))
# print(json.dumps(map_json,indent = 4, sort_keys = True))
lat_arr = [] 
long_arr = [] 
for step in range(len(map_json["routes"][0]["legs"][0]["steps"])):
	lat_arr.append(float(map_json["routes"][0]["legs"][0]["steps"][step]["end_location"]['lat']))
	long_arr.append(float(map_json["routes"][0]["legs"][0]["steps"][step]["end_location"]['lng']))
	# print(map_json["routes"][0]["legs"][0]["steps"][step]["end_location"]['lat'])#, indent=4, sort_keys=True))
	print(json.dumps(map_json["routes"][0]["legs"][0]["steps"][step]["end_location"], indent=4, sort_keys=True))

print("Latitude Array = ",lat_arr)
print("Longitude Array = ", long_arr)