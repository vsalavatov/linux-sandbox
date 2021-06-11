# Sandbox

### Requirements

* `cgroup v2`
* `gcc-10`
* `libcgroup`

### Common issues

* freezer subsystem is not mounted
    ```shell
    $ mkdir cgroup-freezer
    $ sudo mount -t cgroup -ofreezer none cgroup-freezer/ 
    ```
