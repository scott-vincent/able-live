usage="Usage: $0 [enable|disable]"

local="/tmp/pilotaware_data"
path="/pilotaware_data"

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
