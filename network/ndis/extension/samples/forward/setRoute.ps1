#
# Copyright (c) Microsoft Corporation. All Rights Reserved.
#

function StringToMacAddress([String]$MacString)
{
	$zeroVal = [int]("0"[0]);
	$aVal = [int]("A"[0]);

	$macArray = @();
	$byteOne = 0;
	$byteTwo = 0;
	
	for($i = 0; $i -lt 6; $i++)
	{
		$byteChars = $MacString.ToCharArray(2*$i, 2);

		$byteOne = $byteChars[0] - $zeroVal;
		$byteTwo = $byteChars[1] - $zeroVal;

		if($byteOne -gt 9)
		{
			$byteOne = $byteChars[0] - $aVal + 10;
		}
		if($byteTwo -gt 9)
		{
			$byteTwo = $byteChars[1] - $aVal + 10;
		}

		$byteVal = 16*($byteOne) + ($byteTwo);
		$macArray = $macArray + $byteVal;
	}

	return $macArray
}

Import-Module Hyper-V

$vmArr = Get-VM
$switch = Get-VmSwitch -Name "CorpNet"
$policy = Get-VmSystemSwitchExtensionSwitchFeature -FeatureName "MSForwardExt Mac Address Policy"

foreach($vm in $vmArr)
{
    $vmName = $vm.Name
    $adapters = Get-VmNetworkAdapter -VmName $vmName
    
    foreach($adapter in $adapters)
    {
        Write-Host "Setting Policy for $vmName..."
        $policy.SettingData.MacAddress = StringToMacAddress($adapter.MacAddress)
        Add-VmSwitchExtensionSwitchFeature -VmSwitch $switch -VMSwitchExtensionFeature $policy
    }
}
