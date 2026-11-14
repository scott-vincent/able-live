usage="Usage: $0 [enable|disable]"

local="/mem/pilotaware_data"
path="/pilotaware_data"

rtLocal="/mem/radiotelephony_data"
rtPath="/radiotelephony_data"

ipLocal="/mem/teamspeak_ip"
ipPath="/teamspeak_ip"

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

    if [ ! -f $rtLocal ]
    then
        >$rtLocal
    fi
    sudo tailscale funnel --set-path $rtPath -bg "$rtLocal"

    if [ ! -f $ipLocal ]
    then
        >$ipLocal
    fi
    sudo tailscale funnel --set-path $ipPath -bg "$ipLocal"
elif [ "$1" = disable ]
then
    sudo tailscale funnel --set-path $path -bg "$local" off
    sudo tailscale funnel --set-path $rtPath -bg "$rtLocal" off
    sudo tailscale funnel --set-path $ipPath -bg "$ipLocal" off
    tailscale serve status
else
    echo "$usage"
    exit
fi
