Running Open vSwitch under SELinux
==================================

Security-Enhanced Linux (SELinux) is a Linux kernel security
module that limits "the malicious things" that certain processes,
including OVS, can do to the system in case they get compromised.
In our case SELinux basically serves as the "second line of defense"
that limits the things that OVS processes are allowed to do.  The
"first line of defense" is proper input validation that eliminates
code paths that could be used by attacker to do any sort of
"escape attacks" (e.g. file name escape, shell escape, command
line argument escape, buffer escape).  Since developers don't
always implement proper input validation, then SELinux Access
Control's goal is to confine damage of such attacks, if they
turned out to be possible.

Besides Type Enforcement there are other SELinux
features, but they are out of scope for this document.

Currently there are two SELinux policies for Open vSwitch:
1. the one that ships with your Linux distribution (i.e.
   selinux-policy-targeted package); And
2. the one that ships with OVS (i.e. openvswitch-selinux-policy
   package).


Limitations
-----------

If Open vSwitch is directly started from command line, then it
will run under "unconfined_t" SELinux domain that basically lets
daemon to do whatever it likes.  This is very important for developers
to understand, because they might introduced code in OVS that invokes
new system calls that SELinux policy did not anticipate.  This means
that their feature may have worked out just fine for them.  However,
if someone else would try to run the same code when Open vSwitch is
started through systemctl, then Open vSwitch would get Permission Denied
errors.

Currently the only distributions that enforce SELinux on OVS by
default are RHEL, CentOS and Fedora.  While Ubuntu and Debian also
have some SELinux support, they run Open vSwitch under the unrestricted
"unconfined" domain.  Also, it seems that Ubuntu is leaning towards
Apparmor that works slightly differently than SELinux.

SELinux and Open vSwitch are moving targets.  What this means
is that, if you solely rely on your Linux distribution's SELinux policy,
then this policy might not have correctly anticipated that a newer
Open vSwitch version needs extra white list rules.  However, if you
solely rely on SELinux policy that ships with Open vSwitch, then
Open vSwitch developers might not have correctly anticipated the
feature set that your SELinux implementation supports.


Installation
------------

Refer to [INSTALL.Fedora.md] for instructions on how to build all
Open vSwitch rpm packages.

Once the package is built, install it on your Linux distribution with:

   # yum install openvswitch-selinux-policy-2.4.1-1.el7.centos.noarch.rpm

And, then restart Open vSwitch:

   # systemctl restart openvswitch


Troubleshooting
---------------

When SELinux was implemented some of the standard system utilities
acquired "-Z" flag (e.g. "ps -Z", "ls -Z").  For example, to find out
under which SELinux security domain process runs, use:

   # ps -AZ | grep ovs-vswitchd
   system_u:system_r:openvswitch_t:s0 854 ?    ovs-vswitchd

To find out the SELinux label of file or directory, use:

   # ls -Z /etc/openvswitch/conf.db
   system_u:object_r:openvswitch_rw_t:s0 /etc/openvswitch/conf.db


If, for example, SELinux policy for Open vSwitch is too strict,
then you might see in Open vSwitch log files "Permission Denied"
errors:

    # cat /var/log/openvswitch/ovs-vswitchd.log
    vlog|INFO|opened log file /var/log/openvswitch/ovs-vswitchd.log
    ovs_numa|INFO|Discovered 2 CPU cores on NUMA node 0
    ovs_numa|INFO|Discovered 1 NUMA nodes and 2 CPU cores
    reconnect|INFO|unix:/var/run/openvswitch/db.sock: connecting...
    reconnect|INFO|unix:/var/run/openvswitch/db.sock: connected
    netlink_socket|ERR|fcntl: Permission denied
    dpif_netlink|ERR|Generic Netlink family 'ovs_datapath' does not exist.
                     The Open vSwitch kernel module is probably not loaded.
    dpif|WARN|failed to enumerate system datapaths: Permission denied
    dpif|WARN|failed to create datapath ovs-system: Permission denied



However, not all "Permission denied" errors are caused by SELinux.  So,
before blaming too strict SELinux policy, make sure that indeed SELinux
was the one that denied OVS access to certain resources, for example, run:

   # grep "openvswitch_t" /var/log/audit/audit.log | tail
   type=AVC msg=audit(1453235431.640:114671): avc:  denied  { getopt } for  pid=4583 comm="ovs-vswitchd" scontext=system_u:system_r:openvswitch_t:s0 tcontext=system_u:system_r:openvswitch_t:s0 tclass=netlink_generic_socket permissive=0


If SELinux denied OVS access to certain resources, then make sure that you
have installed our SELinux policy package that "loosens" up distribution's
SELinux policy:

   # rpm -qa | grep openvswitch-selinux
   openvswitch-selinux-policy-2.4.1-1.el7.centos.noarch

And, then verify that this module was indeed loaded:

   # semodule -l | grep openvswitch
   openvswitch-custom    1.0
   openvswitch          1.1.1

If you still see Permission denied errors, then take a look
into selinux/openvswitch.te file in the OVS source tree and
try to add white list rules.  This is really simple, just run
SELinux audit2allow tool:

   # grep "openvswitch_t" /var/log/audit/audit.log | audit2allow -M ovslocal

See "Contributing SELinux policy patches" section, if you think
that other Open vSwitch users would benefit from your SELinux policy
changes.


Contributing SELinux policy patches
-----------------------------------

Here are few things to consider before proposing SELinux policy
patches to Open vSwitch developer mailing list:

1. The SELinux policy that resides in Open vSwitch source tree
   amends SELinux policy that ships with your distributions.

   Implications of this are that it is assumed that the distribution's
   Open vSwitch SELinux module must be already loaded to satisfy
   dependencies.

2. The SELinux policy that resides in Open vSwitch source tree
   must work on all currently relevant Linux distributions.

   Implications of this are that you should use only those SELinux
   policy features that are supported by the lowest SELinux version
   out there.  Typically this means that you should test your SELinux
   policy changes on the oldest RHEL or CentOS version that this
   OVS version supports.  Check INSTALL.Fedora.md file to find out
   this.

3. The SELinux policy is enforced only when state transition to
   openvswitch_t domain happens.

   Implications of this are that perhaps instead of loosening SELinux
   policy you can do certain things at the time rpm package is installed.



Reporting Bugs
--------------

Please report problems to bugs@openvswitch.org.

[INSTALL.rst]:INSTALL.rst
