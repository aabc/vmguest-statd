Periodically log VM guest CPU stats to syslog
=============================================

Simple daemon to log CPU stats provided by VMware's vmGuestLib (especially
interesting is steal time and percentage of CPU limit) to syslog for later
analyse of virtual machine performance.

Idea is based on Dag Wieers' vmguest-stats tool from his Python VMGuestLib
wrapper.

Author and license
------------------

This software is (c) 2017 abc@telekom.ru

This program is free software: you can redistribute it and/or modify it under
the terms of the GNU General Public License as published by the Free Software
Foundation, either version 2 of the License, or (at your option) any later
version.
