# Sandbox

### Requirements

* `cgroup v2`

    To use cgroup v2, you might need to change the configuration of the host init system. Fedora (>= 31) uses cgroup v2 by default and no extra configuration is required. On other systemd-based distros, cgroup v2 can be enabled by adding `systemd.unified_cgroup_hierarchy=1` to the kernel cmdline.
    
* `gcc-10`
* `libcgroup`, `libecap`

### Common issues
* `memory.swap.max`  group parameter does not exist

    Swap accounting can be enabled by adding `systemd.unified_cgroup_hierarchy=1` to the kernel cmdline.

* freezer subsystem is not mounted
    ```shell
    $ mkdir cgroup-freezer
    $ sudo mount -t cgroup -ofreezer none cgroup-freezer/ 
    ```
