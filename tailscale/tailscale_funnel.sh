usage="Usage: $0 [enable|disable]"

local="/mem/pilotaware_data"
path="/pilotaware_data"

liveLocal="/mem/able-live.ip"
livePath="/able-live.ip"

tsLocal="/mem/teamspeak.ip"
tsPath="/teamspeak.ip"

# Create RAM drive
if [ ! -d /mem ]
then
  sudo mkdir /mem
  echo tmpfs /mem tmpfs size=1M 0 0 | sudo tee -a /etc/fstab >/dev/null
  sudo mount -a
fi

if [ "$1" = enable ]
then
    if [ ! -f $local ]
    then
        >$local
    fi
    sudo tailscale funnel --set-path $path -bg "$local"

    if [ ! -f $liveLocal ]
    then
        >$liveLocal
    fi
    sudo tailscale funnel --set-path $livePath -bg "$liveLocal"

    if [ ! -f $tsLocal ]
    then
        >$tsLocal
    fi
    sudo tailscale funnel --set-path $tsPath -bg "$tsLocal"
elif [ "$1" = disable ]
then
    sudo tailscale funnel --set-path $path -bg "$local" off
    sudo tailscale funnel --set-path $rtPath -bg "$liveLocal" off
    sudo tailscale funnel --set-path $ipPath -bg "$tsLocal" off
    tailscale serve status
else
    echo "$usage"
    exit
fi
