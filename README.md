Periodically log VM guest CPU stats to syslog
=============================================

Simple daemon to log CPU stats provided by VMware's vmGuestLib (especially
interesting is steal time and percentage of CPU limit) to syslog for later
analyse of virtual machine performance.

It will periodically (once per minute) log to `/var/log/messages` such text:

```
 vmguest-stat: Host freq 2400 MHz, Reserved 1400 MHz, Limit 1400 MHz, CPU shares 2000
 vmguest-stat: CPU used 10.46%, stolen  1.34%, effective freq  251 MHz (10% of limit)
```

Rationale: Usually you don't see `stolen%` on linux guests of vmware using
standard ps-utils. As a consequence, you don't know if it's your system loads
CPU or other guests on the hosts stealing your CPU time. With this tool you
will be able to see that. Also, it is useful to know if you are reaching your
CPU limit (if vmware host is configured to allow you to read this value.)


Idea is based on Dag Wieers' vmguest-stats tool from his Python VMGuestLib
wrapper.

Author and license
------------------

This software is (c) 2017 abc@telekom.ru

This program is free software: you can redistribute it and/or modify it under
the terms of the GNU General Public License as published by the Free Software
Foundation, either version 2 of the License, or (at your option) any later
version.
