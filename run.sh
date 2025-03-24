if [ "$1" != "" ]
then
  echo sleep $1
  sleep $1
fi
log=/home/pi/able-live.log
date >$log
cd /home/pi/able-live
while [ 1 ]
do
  if [ ! -f /usr/local/bin/able-suppress.dat ]
  then
    /home/pi/get-pilotaware-url.sh >>$log
    cd /home/pi/able-live/able-live
    ./able-live >>$log
  fi

  if [ -f /usr/local/bin/able-suppress.dat ]
  then
    echo Paused >>$log
    while [ -f /usr/local/bin/able-suppress.dat ]
    do
      sleep 1
    done
  else
    echo Restarting in 5 seconds >>$log
    sleep 5
  fi
done
