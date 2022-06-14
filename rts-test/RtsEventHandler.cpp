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

	TABLET_CONTEXT_ID* tabletContexts;
	hr = this->rts->GetAllTabletContextIds(&this->num_tablet_contexts, &tabletContexts);
	if (FAILED(hr)) {
		ErrorExit(L"GetAllTabletContextIds", hr);
	}

	UINT tcidx = 0;
	for (UINT i = 0; i < this->num_tablet_contexts; i++) {
		IInkTablet* inkTablet = NULL;

		hr = rts->GetTabletFromTabletContextId(tabletContexts[i], &inkTablet);
		if (FAILED(hr)) {
			ErrorExit(L"GetTabletFromTabletContextId", hr);
		}

		float x = 0, y = 0;
		ULONG packet_props = 0;
		PACKET_PROPERTY* pp;
		hr = rts->GetPacketDescriptionData(tabletContexts[i], &x, &y, &packet_props, &pp);
		if (FAILED(hr)) {
			ErrorExit(L"GetPacketDescriptionData", hr);
		}

		for (UINT j = 0; j < packet_props; j++) {
			if (pp[j].guid != GUID_PACKETPROPERTY_GUID_NORMAL_PRESSURE)
				continue;

			this->tablet_contexts[tcidx].id = tabletContexts[i];
			this->tablet_contexts[tcidx].pressure = j;
			this->tablet_contexts[tcidx].pressure_rcp = 1.0f / pp[j].PropertyMetrics.nLogicalMax;
			tcidx++;
			break;
		}

		CoTaskMemFree(pp);
	}

	std::cout << "nice" << std::endl;
}

void RtsEventHandler::redraw() {
	InvalidateRect(this->hwnd, NULL, true);
}

const char* typeToName(int type) {
	switch (type) {
	case 0:
		return "x";
	case 1:
		return "y";
	case 2:
		return "pressure";
	case 3:
		return "status";
	default:
		return "unknown";
	}
}

void handlePackets(LONG* packetBuf, ULONG packetBufLen, ULONG propsPerPacket) {
	int packetType = 0;
	for (LONG* buf = packetBuf; buf < packetBuf + packetBufLen; buf += propsPerPacket) {
		for (int i = 0; i < propsPerPacket; i++) {
			std::cout << typeToName(packetType) << "=" << buf[i] << " ";
			if (packetType == 3) {
				std::cout << std::endl;
			}

			packetType = (packetType + 1) % 4;
		}
	}
}

STDMETHODIMP RtsEventHandler::Packets(
	IRealTimeStylus*,
	const StylusInfo* si,
	ULONG propsPerPacket,
	ULONG packetBufLen,
	LONG* packetBuf,
	ULONG*,
	LONG**
) {
	if (this->isInverted != si->bIsInvertedCursor) {
		std::cout << "inverted state change" << std::endl;
		this->redraw();
	}

	//handlePackets(packetBuf, packetBufLen, propsPerPacket);

	this->isInverted = si->bIsInvertedCursor;
	return S_OK;
}

STDMETHODIMP RtsEventHandler::InAirPackets(
	IRealTimeStylus*,
	const StylusInfo* si,
	ULONG propsPerPacket,
	ULONG packetBufLen,
	LONG* packetBuf,
	ULONG*,
	LONG**
) {
	if (this->isInverted != si->bIsInvertedCursor) {
		std::cout << "inverted state change" << std::endl;
		this->redraw();
	}

	//handlePackets(packetBuf, packetBufLen, propsPerPacket);

	this->isInverted = si->bIsInvertedCursor;
    return S_OK;
}

STDMETHODIMP RtsEventHandler::StylusDown(IRealTimeStylus*, const StylusInfo* si, ULONG, LONG*, LONG**) {
	if (this->isInverted != si->bIsInvertedCursor) {
		std::cout << "inverted state change" << std::endl;
		this->redraw();
	}

	this->isInverted = si->bIsInvertedCursor;
    return S_OK;
}

STDMETHODIMP RtsEventHandler::StylusUp(IRealTimeStylus*, const StylusInfo* si, ULONG, LONG*, LONG**) {
	if (this->isInverted != si->bIsInvertedCursor) {
		std::cout << "inverted state change" << std::endl;
		this->redraw();
	}

	this->isInverted = si->bIsInvertedCursor;
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
