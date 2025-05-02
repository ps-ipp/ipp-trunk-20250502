
macro load.hosts
  parameters unwant = 1
  parameters want = 2.0

  host add pikake
  host add pikake
  host add ipp022
  host add ipp022
  host add ipp022
  host add ipp022
  host add ipp002
  host off ipp002

  machines
end

macro load.jobs
  job -host pikake sleep 10
  job -host pikake sleep 10
  job -host pikake sleep 10
  job -host pikake sleep 10
  job -host pikake sleep 10
  job -host pikake sleep 10
  job -host pikake sleep 10
  job -host pikake sleep 10
end

macro load.xhost
  job -host pikake -xhost ipp002 sleep 10
  job -host pikake -xhost ipp002 sleep 10
  job -host ipp022 -xhost ipp002 sleep 10
  job -host ipp022 -xhost ipp002 sleep 10
  job -host pikake -xhost ipp002 sleep 10
  job -host pikake -xhost ipp002 sleep 10
  job -host ipp022 -xhost ipp002 sleep 10
  job -host ipp022 -xhost ipp002 sleep 10
end

macro load.2xhost
  job -host pikake -xhost ipp002 -xhost ipp022 sleep 10
  job -host pikake -xhost ipp002 -xhost ipp022 sleep 10
  job -host ipp022 -xhost ipp002 -xhost ipp022 sleep 10
  job -host ipp022 -xhost ipp002 -xhost ipp022 sleep 10
  job -host pikake -xhost ipp002 -xhost ipp022 sleep 10
  job -host pikake -xhost ipp002 -xhost ipp022 sleep 10
  job -host ipp022 -xhost ipp002 -xhost ipp022 sleep 10
  job -host ipp022 -xhost ipp002 -xhost ipp022 sleep 10
end

macro load.hosts.zombie
  parameters connect = 2.0

  host add pikake
  host add pikake
  host add ipp022
  host add ipp022
  host add ipp022
  host add ipp022

  machines
end

macro load.jobs.zombie
  job sleep 10
  job sleep 10
  job sleep 10
  job sleep 10
  job sleep 10
  job sleep 10
  job sleep 10
  job sleep 10
end

