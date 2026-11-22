urlFile=/home/pi/pilotaware.uri
#echo "https://PWEGLK:8aa02e3d5d@data.pilotaware.com/PWEGLK/vrs/VirtualRadar/AircraftList.json" >$urlFile
#echo "http://172.16.16.87:9000/VirtualRadar/AircraftList.json" >$urlFile

out=`ping -4 -c 1 -w 1 ognpaw 2>/dev/null`
if [ $? -eq 0 ]
then
  ip=`echo $out|head -1|cut -f 3 -d ' '|tr -d '()'`
  echo "http://$ip:9000/VirtualRadar/AircraftList.json" >$urlFile
  echo "Using local PilotAware"
else
  echo "Local PilotAware not found"
  echo "https://PWEGLK:8aa02e3d5d@data.pilotaware.com/PWEGLK/vrs/VirtualRadar/AircraftList.json" >$urlFile
fi
