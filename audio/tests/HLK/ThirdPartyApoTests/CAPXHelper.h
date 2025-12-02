#pragma once

#include <stdafx.h>

#include <wil\resultmacros.h>
#include <wil\com.h>
#include <audioclient.h>
#include <audioclientp.h>
#include <audiosessiontypesp.h>
#include <mmdeviceapi.h>
#include <mmdeviceapip.h>
#include <endpointvolume.h>
#include "etwlistener.h"
#include <functiondiscoverykeys.h>
#include <slpublic.h>

#include <DevPKey.h>
#include <AudioEngineBaseAPOP.h>
#include "TestMediaType.h"
#include <audioenginebaseapo.h>

#include <list>
#include <memory>

using namespace std;
using namespace wil;
using namespace WEX::Logging;
using namespace WEX::Common;

bool IsCapXAPO(GUID clsid);
void GetClsidsFromVar(const PROPVARIANT& var, std::vector<GUID>& guids, _Outptr_ PWSTR* ppszFormattedClsids, std::vector<GUID>& capxGuids, _Outptr_ PWSTR* ppszCapXFormattedClsids);
bool EndpointContainsCapx(IMMDevice* pDevice);
void EndpointsWithCapx(std::vector<wil::com_ptr<IMMDevice>>& capxEndpoints);