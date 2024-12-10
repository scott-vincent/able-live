if [ "$1" != "" ]
then
  echo sleep $1
  sleep $1
fi
cd /home/pi/able-live
while [ 1 ]
do
  if [ ! -f /usr/local/bin/able-suppress.dat ]
  then
    ./run.sh >>/home/pi/able-live.log
  fi

  if [ -f /usr/local/bin/able-suppress.dat ]
  then
    echo Paused
    while [ -f /usr/local/bin/able-suppress.dat ]
    do
      sleep 1
    done
  else
    echo Restarting in 5 seconds
    sleep 5
  fi
done
