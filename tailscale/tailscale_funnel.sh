usage="Usage: $0 [enable|disable]"

local="/mem/pilotaware_data"
path="/pilotaware_data"

# Create RAM drive
if [ ! -d /mem ]
then
  sudo mkdir /mem
  echo tmpfs /mem tmpfs nodev,nosuid,size=32K 0 0 | sudo tee -a /etc/fstab >/dev/null
  sudo mount -a
fi

if [ "$1" = enable ]
then
    if [ ! -f $local ]
    then
        >$local
    fi
    sudo tailscale funnel --set-path $path -bg "$local"
elif [ "$1" = disable ]
then
    sudo tailscale funnel --set-path $path -bg "$local" off
    tailscale serve status
else
    echo "$usage"
    exit
fi
