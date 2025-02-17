# Network management

There are two independent networks: one for app communications (connected via
PS EMAC on MIO) and one for dante (connected via Dante IP Core in PL). Some
care needs to be taken to manage these.

## Pleasing the IP Core
The dante IP core is hardcoded such that dante primary always wants to be
`eth0`, and secondary `eth1`. This is a hardcoded assumption throughout the
drivers (and presumable the userland container). This means that we must always
allow dante to claim eth0 and eth1, even if it starts long after the PS EMAC
network interface.

Here is the current startup sequence:
1. Zynq PS EMAC driver is compiled in-tree with the kernel (not module), so it
   is probed immediately when the device tree is walked.
2. Zynq PS EMAC platform device creates a child network device, claiming `eth0`
3. At this point, udev has not been started as /init has not been run yet.
   Therefore no kernel modules have been loaded, including akashi-temac dante
   ethernet device.
4. Kernel runs /init
5. udevd is started by init system
6. **Before** the udev rule for loading kernel modules runs, another rule runs,
   which renames the Zynq PS MAC ethernet device from `eth0` to `ethps0`. This
   frees `eth0` for use by dante.
7. The udev rule for loading kernel modules is run. This checks the remaining
   device tree entries, including Dante, and probes them..
8. Dante can now claim `eth0` and `eth1`, allowing the inferno to commence.


## Network isolation
This is still being worked out. We need to ensure the app comms can never
transmit out of the dante ports and vice-versa, even with IP address conflicts.
Network namespaces will likely be the solution.
