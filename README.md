<a href="https://scan.coverity.com/projects/facron">
  <img alt="Coverity Scan Build Status"
       src="https://scan.coverity.com/projects/6314/badge.svg"/>
</a>

facron is a new generation filesystem cron

See <http://www.imagination-land.org/posts/2012-12-04-facron-fanotify-cron-system.html> and
<http://www.imagination-land.org/posts/2012-12-07-facron-released.html> for more details.

Latest release is [1.1](http://www.imagination-land.org/files/facron/facron-1.1.tar.xz)

To build facron:

```
git clone git://github.com/Keruspe/facron.git
cd facron
./autogen.sh
./configure --sysconfdir=/etc --with-systemdsystemunitdir=/usr/lib/systemd/system
make
sudo make install
```

facron configuration file is `/etc/facron.conf`.
You can put as many entries as you want in this file, one entry per line.
Each line must be formatted like this:

<file path> <fanotify masks> <command>

Each time we receive an event matching the fanotify masks on the file path given, the
command is launched.

The fanotify masks available are:

 - `FAN_ACCESS`
 - `FAN_MODIFY`
 - `FAN_CLOSE_WRITE`
 - `FAN_CLOSE_NOWRITE`
 - `FAN_OPEN`
 - `FAN_Q_OVERFLOW`
 - `FAN_OPEN_PERM`
 - `FAN_ACCESS_PERM`
 - `FAN_ONDIR`
 - `FAN_EVENT_ON_CHILD`
 - `FAN_CLOSE`
 - `FAN_ALL_EVENTS`
 - `FAN_ALL_PERM_EVENTS`
 - `FAN_ALL_OUTGOING_EVENTS`

If you configure your fanotify masks like this:

```
FAN_MODIFY|FAN_CLOSE_WRITE,FAN_OPEN
```

The event caught will be either `FAN_MODIFY` AND `FAN_CLOSE_WRITE`, or `FAN_OPEN`

The command should be an absolute path. You can pass it arguments.
If any of your arguments containis sapces, you can surround it with quotes or double quotes.
Four special arguments are available:

 - `$@` corresponds to the dirname of your file
 - `$#` corresponds to the basename of your file
 - `$$` corresponds to the full path of your file
 - `$*` corresponds to the process id that accessed the file

Three additional arguments are available which handle a global counter:

 - `$+` increments the counter and returns its new value
 - `$-` decrements the counter and returns its new value
 - `$=` returns its value

You can reload the configuration at any time by sending a SIGUSR1 to facron:

```
kill -USR1 $(pidof facron)
```
