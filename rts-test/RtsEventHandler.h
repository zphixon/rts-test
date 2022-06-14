#pragma once

#include <rtscom.h>
#include <rtscom_i.c>

class RtsEventHandler : public IStylusSyncPlugin {
private:
	void RealInit();

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

	LONG m_nRef;
	IUnknown* m_pPunkFTMarshaller;

	HWND hwnd;
	IRealTimeStylus* rts;

	LONG x;
	LONG y;
	LONG buttonStatus;
	LONG pressure;

	RtsEventHandler(HWND hwnd)
		: m_nRef(1)
		, m_pPunkFTMarshaller(NULL)
		, hwnd(hwnd)
		, rts(NULL)
		, x(-1)
		, y(-1)
		, buttonStatus(-1)
		, pressure(-1)
	{
		RealInit();
	}

	virtual ~RtsEventHandler() {
		if (m_pPunkFTMarshaller != NULL)
			m_pPunkFTMarshaller->Release();

		if (rts != NULL) {
			rts->Release();
		}
	}

	void handlePackets(
		IRealTimeStylus* stylus,
		const StylusInfo* si,
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
		IRealTimeStylus* pStylus,
		const StylusInfo* pStylusInfo,
		ULONG,
		ULONG nPacketBuf,
		LONG* pPackets,
		ULONG*,
		LONG**
	);

    STDMETHOD(InAirPackets)(
		IRealTimeStylus* pStylus,
		const StylusInfo* pStylusInfo,
		ULONG,
		ULONG nPacketBuf,
		LONG* pPackets,
		ULONG*,
		LONG**
	);

	STDMETHOD_(ULONG, AddRef)() {
		return InterlockedIncrement(&m_nRef);
	}

	STDMETHOD_(ULONG, Release)() {
		ULONG nNewRef = InterlockedDecrement(&m_nRef);
		if (nNewRef == 0) {
			delete this;
		}
		return nNewRef;
	}

	STDMETHOD(QueryInterface)(REFIID riid, LPVOID* ppvObj) {
		if (riid == IID_IStylusSyncPlugin || riid == IID_IUnknown) {
			*ppvObj = this;
			AddRef();
			return S_OK;
		}
		else if (riid == IID_IMarshal && m_pPunkFTMarshaller != NULL) {
			return m_pPunkFTMarshaller->QueryInterface(riid, ppvObj);
		}

		*ppvObj = NULL;
		return E_NOINTERFACE;
	}

	STDMETHOD(StylusButtonUp)(IRealTimeStylus*, STYLUS_ID, const GUID*, POINT*);
    STDMETHOD(StylusButtonDown)(IRealTimeStylus*, STYLUS_ID, const GUID*, POINT*);
	STDMETHOD(StylusInRange)(IRealTimeStylus*, TABLET_CONTEXT_ID, STYLUS_ID);
	STDMETHOD(StylusOutOfRange)(IRealTimeStylus*, TABLET_CONTEXT_ID, STYLUS_ID);
    STDMETHOD(StylusDown)(IRealTimeStylus*, const StylusInfo*, ULONG, LONG* _pPackets, LONG**);
    STDMETHOD(StylusUp)(IRealTimeStylus*, const StylusInfo*, ULONG, LONG* _pPackets, LONG**);
    STDMETHOD(RealTimeStylusEnabled)(IRealTimeStylus*, ULONG, const TABLET_CONTEXT_ID*);
    STDMETHOD(RealTimeStylusDisabled)(IRealTimeStylus*, ULONG, const TABLET_CONTEXT_ID*);
    STDMETHOD(SystemEvent)(IRealTimeStylus*, TABLET_CONTEXT_ID, STYLUS_ID, SYSTEM_EVENT, SYSTEM_EVENT_DATA);
    STDMETHOD(TabletAdded)(IRealTimeStylus*, IInkTablet*);
    STDMETHOD(TabletRemoved)(IRealTimeStylus*, LONG);
    STDMETHOD(CustomStylusDataAdded)(IRealTimeStylus*, const GUID*, ULONG, const BYTE*);
    STDMETHOD(Error)(IRealTimeStylus*, IStylusPlugin*, RealTimeStylusDataInterest, HRESULT, LONG_PTR*);
    STDMETHOD(UpdateMapping)(IRealTimeStylus*);
};

