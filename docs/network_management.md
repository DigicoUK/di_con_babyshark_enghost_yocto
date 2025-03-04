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
8. Dante can now claim `eth0` and `eth1`.


## Network isolation
Services like the engine comms link run one or more TCP servers that accept all
incoming connections. This need to be isolated from the dante network to avoid
external devices from attempting to send parameters to the engine and causing
corruption.

On initialization, a network namespace `appcomms` is created. The `ethps0`
ethernet device is moved in to this namespace so that it is no longer visible
in the root/default namespace. All processes that need to interact with this
network device need to be started in this namespace using
`ip netns exec appcomms <cmd...>`. To avoid verbosity when doing a lot of work
with the `ethps0` link, `ip netns exec appcomms sh` can be used to start a new
shell in the namespace. Dropbear SSH client is also started in this namespace,
meaning it will not be accessible from the dante ports.

Ideally, the dante IP core would be started in it's own namespace keeping it
isolated to the dante ports, but this is more trouble than it's worth since the
IP core is already an OCI container. Network namespace support can be added to
the container by modifying
`/tmp/ipcore/dante_data/capability/config.khaju.json`, but this would need to
be patched for each new dante IP core update and is not necessarily a stable
patch as it's controlled by audinate. Because of this I've chosen to run the
ipcore in the default root namespace.

## Diagram
