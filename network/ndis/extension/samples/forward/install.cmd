netcfg -l .\MSForwardExt.inf -c s -i ms_forwardext

net stop vmms
mofcomp -N:root\virtualization\v2 .\MSForwardExtPolicy.mof
mofcomp -N:root\virtualization\v2 .\MSForwardExtPolicyStatus.mof
net start vmms
