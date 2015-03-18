#
# Copyright (c) Microsoft Corporation. All Rights Reserved.
#

Import-Module Hyper-V

$switch = Get-VmSwitch -Name "CorpNet"
$features = Get-VmSwitchExtensionSwitchFeature -VmSwitch $switch -FeatureName "MSForwardExt Mac Address Policy"
if ($features -ne $null)
{
    Remove-VmSwitchExtensionSwitchFeature -VmSwitch $switch -VmSwitchExtensionFeature $features
}