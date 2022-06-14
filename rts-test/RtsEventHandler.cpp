#include "RtsEventHandler.h"
#include "errorexit.h"

#include <iostream>

void RtsEventHandler::RealInit() {
	HRESULT hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
	if (FAILED(hr)) {
		ErrorExit(L"CoInitializeEx", hr);
	}

	hr = CoCreateInstance(CLSID_RealTimeStylus, NULL, CLSCTX_ALL, IID_PPV_ARGS(&this->rts));
	if (FAILED(hr)) {
		ErrorExit(L"CoCreateInstance", hr);
	}

	hr = this->rts->put_HWND((HANDLE_PTR)this->hwnd);
	if (FAILED(hr)) {
		ErrorExit(L"put_HWND", hr);
	}

	hr = CoCreateFreeThreadedMarshaler(this, &this->m_pPunkFTMarshaller);
	if (FAILED(hr)) {
		ErrorExit(L"CoCreateFreeThreadedMarshaler", hr);
	}

	hr = this->rts->AddStylusSyncPlugin(0, this);
	if (FAILED(hr)) {
		ErrorExit(L"AddStylusSyncPlugin", hr);
	}

	const UINT numProps = 4;
	GUID lWantedProps[numProps] = {
		GUID_PACKETPROPERTY_GUID_X,
		GUID_PACKETPROPERTY_GUID_Y,
		GUID_PACKETPROPERTY_GUID_NORMAL_PRESSURE,
		GUID_PACKETPROPERTY_GUID_PACKET_STATUS,
	};

	hr = this->rts->SetDesiredPacketDescription(numProps, lWantedProps);
	if (FAILED(hr)) {
		ErrorExit(L"SetDesiredPacketDescription", hr);
	}

	hr = this->rts->put_Enabled(true);
	if (FAILED(hr)) {
		ErrorExit(L"put_Enabled", hr);
	}
}

void RtsEventHandler::redraw() {
	InvalidateRect(this->hwnd, NULL, true);
	UpdateWindow(this->hwnd);
}

void RtsEventHandler::readPackets(
	LONG* packetBuf,
	ULONG packetBufLen,
	ULONG nProps,
	struct PacketDescription pd
) {
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

			this->buttonStatus = prop;

			if (shouldRedraw) {
				this->redraw();
			}
		}
	}
}

void RtsEventHandler::handlePackets(
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
STDMETHODIMP RtsEventHandler::Packets(
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
STDMETHODIMP RtsEventHandler::InAirPackets(
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

STDMETHODIMP RtsEventHandler::StylusDown(IRealTimeStylus*, const StylusInfo* si, ULONG, LONG*, LONG**) {
    return S_OK;
}

STDMETHODIMP RtsEventHandler::StylusUp(IRealTimeStylus*, const StylusInfo* si, ULONG, LONG*, LONG**) {
    return S_OK;
}

STDMETHODIMP RtsEventHandler::StylusInRange(IRealTimeStylus* s, TABLET_CONTEXT_ID tcid, STYLUS_ID sid) {
	return S_OK;
}

STDMETHODIMP RtsEventHandler::StylusOutOfRange(IRealTimeStylus*, TABLET_CONTEXT_ID, STYLUS_ID) {
	return S_OK;
}

STDMETHODIMP RtsEventHandler::RealTimeStylusEnabled(IRealTimeStylus*, ULONG, const TABLET_CONTEXT_ID*) {
    return S_OK;
}

STDMETHODIMP RtsEventHandler::RealTimeStylusDisabled(IRealTimeStylus*, ULONG, const TABLET_CONTEXT_ID*) {
    return S_OK;
}

STDMETHODIMP RtsEventHandler::StylusButtonDown(IRealTimeStylus*, STYLUS_ID, const GUID*, POINT*) {
	return S_OK;
}

STDMETHODIMP RtsEventHandler::StylusButtonUp(IRealTimeStylus*, STYLUS_ID, const GUID*, POINT*) {
	return S_OK;
}

STDMETHODIMP RtsEventHandler::SystemEvent(IRealTimeStylus*, TABLET_CONTEXT_ID, STYLUS_ID, SYSTEM_EVENT, SYSTEM_EVENT_DATA) {
    return S_OK;
}

STDMETHODIMP RtsEventHandler::TabletAdded(IRealTimeStylus*, IInkTablet*) {
    return S_OK;
}

STDMETHODIMP RtsEventHandler::TabletRemoved(IRealTimeStylus*, LONG) {
    return S_OK;
}

STDMETHODIMP RtsEventHandler::CustomStylusDataAdded(IRealTimeStylus*, const GUID*, ULONG, const BYTE*) {
    return S_OK;
}

STDMETHODIMP RtsEventHandler::Error(IRealTimeStylus*, IStylusPlugin*, RealTimeStylusDataInterest, HRESULT, LONG_PTR*) {
    return S_OK;
}

STDMETHODIMP RtsEventHandler::UpdateMapping(IRealTimeStylus*) {
    return S_OK;
}
