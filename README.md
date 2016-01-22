# rlcmp
`rlcmp` is simple recursive file compare tool which (in contrast to `diff`(1)) never follows symbolic links.
The tool can be used to check the integrity of local backups or doing verified data moves between different
local file systems with a sequence `cp -R`, `rlcmp`, `rm -rf`.

Due to the limited functionality of `rlcmp` it is about ten times faster than `diff`(1).
So for simple file or directory integrity tests with just a pass/fail result it can be used as a fast alternative.

Additional information can be found in the [manual page](http://n-t-roff.github.io/rlcmp.1.html)
and the [installation instructions](https://github.com/n-t-roff/rlcmp/blob/master/INSTALL).

The tool can be downloaded as a
[release tbz file](https://github.com/n-t-roff/rlcmp/releases/download/v1.0.0/rlcmp.tar.bz2),
or with the [Download ZIP](https://github.com/n-t-roff/rlcmp/archive/master.zip) button,
or the command
```
git clone https://github.com/n-t-roff/rlcmp.git
```
In the latter case the archive can be updated with
```
git pull
```
later.
