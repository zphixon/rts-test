#pragma once

#include <rtscom.h>
#include <rtscom_i.c>

class MyRtsPlugin : public IStylusPlugin {
private:
	void realInit();

public:
	enum TouchStatus {
		NotPresent = -1,
		Touching = 1,
		Inverted = 2,
		Unused = 4,
		Barrel = 8,
	};

	struct PacketDescription {
		int idxX;
		int idxY;
		int idxPressure;
		int idxStatus;
	};

	LONG refCount;

	HWND hwnd;
	IRealTimeStylus* rts;

	LONG x;
	LONG y;
	LONG buttonStatus;
	LONG pressure;

	MyRtsPlugin(HWND hwnd)
		: refCount(1)
		, hwnd(hwnd)
		, rts(NULL)
		, x(-1)
		, y(-1)
		, buttonStatus(-1)
		, pressure(-1)
	{
		realInit();
	}

	virtual ~MyRtsPlugin() {
		if (rts != NULL) {
			rts->Release();
		}
	}

	void handlePackets(
		IRealTimeStylus*,
		const StylusInfo*,
		ULONG packetBufLen,
		LONG* packetBuf
	);
	void readPackets(
		LONG* packetBuf,
		ULONG packetBufLen,
		ULONG nProps,
		struct PacketDescription
	);
	void redraw();

	STDMETHOD(DataInterest)(RealTimeStylusDataInterest* pEventInterest) {
		*pEventInterest = (RealTimeStylusDataInterest)(RTSDI_AllData);
		return S_OK;
	}

	STDMETHOD(Packets)(
		IRealTimeStylus*,
		const StylusInfo*,
		ULONG,
		ULONG packetBufLen,
		LONG* packetBuf,
		ULONG*,
		LONG**
	);

    STDMETHOD(InAirPackets)(
		IRealTimeStylus*,
		const StylusInfo*,
		ULONG,
		ULONG packetBufLen,
		LONG* packetBuf,
		ULONG*,
		LONG**
	);

	STDMETHOD_(ULONG, AddRef)() {
		return InterlockedIncrement(&refCount);
	}

	STDMETHOD_(ULONG, Release)() {
		ULONG nNewRef = InterlockedDecrement(&refCount);
		if (nNewRef == 0) {
			delete this;
		}
		return nNewRef;
	}

	STDMETHOD(QueryInterface)(REFIID riid, LPVOID* obj) {
		if (riid == IID_IStylusAsyncPlugin || riid == IID_IUnknown) {
			*obj = this;
			AddRef();
			return S_OK;
		}

		*obj = NULL;
		return E_NOINTERFACE;
	}

	STDMETHOD(StylusButtonUp)(IRealTimeStylus*, STYLUS_ID, const GUID*, POINT*);
    STDMETHOD(StylusButtonDown)(IRealTimeStylus*, STYLUS_ID, const GUID*, POINT*);
	STDMETHOD(StylusInRange)(IRealTimeStylus*, TABLET_CONTEXT_ID, STYLUS_ID);
	STDMETHOD(StylusOutOfRange)(IRealTimeStylus*, TABLET_CONTEXT_ID, STYLUS_ID);
    STDMETHOD(StylusDown)(IRealTimeStylus*, const StylusInfo*, ULONG, LONG*, LONG**);
    STDMETHOD(StylusUp)(IRealTimeStylus*, const StylusInfo*, ULONG, LONG*, LONG**);
    STDMETHOD(RealTimeStylusEnabled)(IRealTimeStylus*, ULONG, const TABLET_CONTEXT_ID*);
    STDMETHOD(RealTimeStylusDisabled)(IRealTimeStylus*, ULONG, const TABLET_CONTEXT_ID*);
    STDMETHOD(SystemEvent)(IRealTimeStylus*, TABLET_CONTEXT_ID, STYLUS_ID, SYSTEM_EVENT, SYSTEM_EVENT_DATA);
    STDMETHOD(TabletAdded)(IRealTimeStylus*, IInkTablet*);
    STDMETHOD(TabletRemoved)(IRealTimeStylus*, LONG);
    STDMETHOD(CustomStylusDataAdded)(IRealTimeStylus*, const GUID*, ULONG, const BYTE*);
    STDMETHOD(Error)(IRealTimeStylus*, IStylusPlugin*, RealTimeStylusDataInterest, HRESULT, LONG_PTR*);
    STDMETHOD(UpdateMapping)(IRealTimeStylus*);
};

