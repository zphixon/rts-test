#include "MyRtsPlugin.h"
#include "errorexit.h"

#include <iostream>

void MyRtsPlugin::realInit() {
	HRESULT hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
	if (FAILED(hr)) ErrorExit(L"CoInitializeEx", hr);

	hr = CoCreateInstance(CLSID_RealTimeStylus, NULL, CLSCTX_ALL, IID_PPV_ARGS(&this->rts));
	if (FAILED(hr)) ErrorExit(L"CoCreateInstance", hr);

	hr = this->rts->put_HWND((HANDLE_PTR)this->hwnd);
	if (FAILED(hr)) ErrorExit(L"put_HWND", hr);

	// "Plugins must aggregate the free threaded marshaler and must not be single threaded apartment objects."
	// https://docs.microsoft.com/en-us/windows/win32/api/rtscom/nf-rtscom-irealtimestylus-addstylussyncplugin
	hr = CoCreateFreeThreadedMarshaler(this, &this->marshaler);
	if (FAILED(hr))	ErrorExit(L"CoCreateFreeThreadedMarshaler", hr);

	hr = this->rts->AddStylusSyncPlugin(0, this);
	if (FAILED(hr)) ErrorExit(L"AddStylusSyncPlugin", hr);

	// https://docs.microsoft.com/en-us/windows/win32/tablet/packetpropertyguids-constants
	const UINT numProps = 4;
	GUID wantedProps[numProps] = {
		GUID_PACKETPROPERTY_GUID_X,
		GUID_PACKETPROPERTY_GUID_Y,
		GUID_PACKETPROPERTY_GUID_NORMAL_PRESSURE,
		GUID_PACKETPROPERTY_GUID_PACKET_STATUS,
	};

	hr = this->rts->SetDesiredPacketDescription(numProps, wantedProps);
	if (FAILED(hr)) ErrorExit(L"SetDesiredPacketDescription", hr);

	hr = this->rts->put_Enabled(true);
	if (FAILED(hr)) ErrorExit(L"put_Enabled", hr);
}

void MyRtsPlugin::redraw() {
	InvalidateRect(this->hwnd, NULL, true);
	UpdateWindow(this->hwnd);
}

void MyRtsPlugin::readPackets(
	LONG* packetBuf,
	ULONG packetBufLen,
	ULONG nProps,
	struct PacketDescription pd
) {
	// the fields of pd are -1 if the packet does not contain that info
	this->x = pd.idxX;
	this->y = pd.idxY;
	this->pressure = pd.idxPressure;
	this->buttonStatus = pd.idxStatus;

	for (int i = 0; i < packetBufLen; i++) {
		LONG prop = packetBuf[i];
		int idx = i % nProps;

		// guh
		// the mouse has x, y, no pressure, and I presume button status
		// and since we ask for them in that order, I'm a little bit sure
		// that we just get x, y, and button with no pressure, which makes
		// this a little awkward

		if (idx == pd.idxX) {
			this->x = prop;
		}
		else if (idx == pd.idxY) {
			this->y = prop;
		}
		else if (idx == pd.idxPressure) {
			this->pressure = prop;
		}
		else if (idx == pd.idxStatus) {
			bool shouldRedraw = false;
			if (this->buttonStatus != prop) {
				shouldRedraw = true;
			}

			// set the new button status *before* redrawing
			this->buttonStatus = prop;

			if (shouldRedraw) {
				this->redraw();
			}
		}
	}
}

void MyRtsPlugin::handlePackets(
	IRealTimeStylus* stylus,
	const StylusInfo* si,
	ULONG packetBufLen,
	LONG* packetBuf
) {
	ULONG nProps = 0;
	PACKET_PROPERTY* props = NULL;
	HRESULT hr = stylus->GetPacketDescriptionData(si->tcid, NULL, NULL, &nProps, &props);
	if (FAILED(hr)) ErrorExit(L"GetPacketDescriptionData", hr);

	struct PacketDescription pd;
	pd.idxX = -1;
	pd.idxY = -1;
	pd.idxStatus = -1;
	pd.idxPressure = -1;

	for (int i = 0; i < nProps; i++) {
		if (props[i].guid == GUID_PACKETPROPERTY_GUID_X) {
			pd.idxX = i;
		}
		else if (props[i].guid == GUID_PACKETPROPERTY_GUID_Y) {
			pd.idxY = i;
		}
		else if (props[i].guid == GUID_PACKETPROPERTY_GUID_PACKET_STATUS) {
			pd.idxStatus = i;
		}
		else if (props[i].guid == GUID_PACKETPROPERTY_GUID_NORMAL_PRESSURE) {
			pd.idxPressure = i;
		}
	}

	readPackets(packetBuf, packetBufLen, nProps, pd);
}

// the stylus is pressed to the tablet
STDMETHODIMP MyRtsPlugin::Packets(
	IRealTimeStylus* stylus,
	const StylusInfo* si,
	ULONG,
	ULONG packetBufLen,
	LONG* packetBuf,
	ULONG*,
	LONG**
) {
	handlePackets(
		stylus,
		si,
		packetBufLen,
		packetBuf
	);

	return S_OK;
}

// the stylus is hovering above the tablet
STDMETHODIMP MyRtsPlugin::InAirPackets(
	IRealTimeStylus* stylus,
	const StylusInfo* si,
	ULONG,
	ULONG packetBufLen,
	LONG* packetBuf,
	ULONG*,
	LONG**
) {
	// in our case, having these methods split is redundant since we ask for
	// the button status anyway, which contains that information
	handlePackets(
		stylus,
		si,
		packetBufLen,
		packetBuf
	);

    return S_OK;
}

STDMETHODIMP MyRtsPlugin::StylusDown(IRealTimeStylus*, const StylusInfo*, ULONG, LONG*, LONG**) {
    return S_OK;
}

STDMETHODIMP MyRtsPlugin::StylusUp(IRealTimeStylus*, const StylusInfo*, ULONG, LONG*, LONG**) {
    return S_OK;
}

STDMETHODIMP MyRtsPlugin::StylusInRange(IRealTimeStylus*, TABLET_CONTEXT_ID, STYLUS_ID) {
	return S_OK;
}

STDMETHODIMP MyRtsPlugin::StylusOutOfRange(IRealTimeStylus*, TABLET_CONTEXT_ID, STYLUS_ID) {
	return S_OK;
}

STDMETHODIMP MyRtsPlugin::RealTimeStylusEnabled(IRealTimeStylus*, ULONG, const TABLET_CONTEXT_ID*) {
    return S_OK;
}

STDMETHODIMP MyRtsPlugin::RealTimeStylusDisabled(IRealTimeStylus*, ULONG, const TABLET_CONTEXT_ID*) {
    return S_OK;
}

STDMETHODIMP MyRtsPlugin::StylusButtonDown(IRealTimeStylus*, STYLUS_ID, const GUID*, POINT*) {
	return S_OK;
}

STDMETHODIMP MyRtsPlugin::StylusButtonUp(IRealTimeStylus*, STYLUS_ID, const GUID*, POINT*) {
	return S_OK;
}

STDMETHODIMP MyRtsPlugin::SystemEvent(IRealTimeStylus*, TABLET_CONTEXT_ID, STYLUS_ID, SYSTEM_EVENT, SYSTEM_EVENT_DATA) {
    return S_OK;
}

STDMETHODIMP MyRtsPlugin::TabletAdded(IRealTimeStylus*, IInkTablet*) {
    return S_OK;
}

STDMETHODIMP MyRtsPlugin::TabletRemoved(IRealTimeStylus*, LONG) {
    return S_OK;
}

STDMETHODIMP MyRtsPlugin::CustomStylusDataAdded(IRealTimeStylus*, const GUID*, ULONG, const BYTE*) {
    return S_OK;
}

STDMETHODIMP MyRtsPlugin::Error(IRealTimeStylus*, IStylusPlugin*, RealTimeStylusDataInterest, HRESULT, LONG_PTR*) {
    return S_OK;
}

STDMETHODIMP MyRtsPlugin::UpdateMapping(IRealTimeStylus*) {
    return S_OK;
}
