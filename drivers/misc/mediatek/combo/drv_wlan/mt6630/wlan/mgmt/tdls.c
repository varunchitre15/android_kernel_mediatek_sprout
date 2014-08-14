/*
** $Id: tdls.c#1 $
*/

/*! \file tdls.c
    \brief This file includes IEEE802.11z TDLS support.
*/


/*
** $Log: tdls.c $
 *
 * 11 13 2013 vend_samp.lin
 * NULL
 * Initial version.
 */

/*******************************************************************************
 *						C O M P I L E R	 F L A G S
 ********************************************************************************
 */

/*******************************************************************************
 *						E X T E R N A L	R E F E R E N C E S
 ********************************************************************************
 */
#include "precomp.h"

#if CFG_SUPPORT_TDLS
#include "tdls.h"
#include "gl_cfg80211.h"

/*******************************************************************************
*						C O N S T A N T S
********************************************************************************
*/
	/* The list of valid data rates. */
/* The list of valid data rates. */

/*******************************************************************************
*						F U N C T I O N   D E C L A R A T I O N S
********************************************************************************
*/


/*******************************************************************************
*						P R I V A T E   D A T A
********************************************************************************
*/

/*******************************************************************************
*						P R I V A T E  F U N C T I O N S
********************************************************************************
*/


#define ELEM_ID_LINK_IDENTIFIER_LENGTH 16


#define	MAXNUM_TDLS_PEER 4
extern UINT_8 g_arTdlsLink[MAXNUM_TDLS_PEER];




/*----------------------------------------------------------------------------*/
/*!
* \brief This routine is called to hadel TDLS link from nl80211.
*
* \param[in] pvAdapter Pointer to the Adapter structure.
* \param[in]
* \param[in]
* \param[in] buf includes RSN IE + FT IE + Lifetimeout IE
*
* \retval WLAN_STATUS_SUCCESS
* \retval WLAN_STATUS_INVALID_LENGTH
*/
/*----------------------------------------------------------------------------*/
UINT_32
TdlsexLinkOper(P_ADAPTER_T prAdapter,
	       PVOID pvSetBuffer, UINT_32 u4SetBufferLen, PUINT_32 pu4SetInfoLen)
{
	/* from supplicant -- wpa_supplicant_tdls_peer_addset() */
	UINT_16 i;
	STA_RECORD_T *prStaRec;


	TDLS_CMD_LINK_OPER_T *prCmd;
	prCmd = (TDLS_CMD_LINK_OPER_T *) pvSetBuffer;

	switch (prCmd->oper) {

	case TDLS_ENABLE_LINK:

		for (i = 0; i < MAXNUM_TDLS_PEER; i++) {
			if (!g_arTdlsLink[i]) {
				g_arTdlsLink[i] = 1;
				prStaRec =
				    cnmGetTdlsPeerByAddress(prAdapter,
							    prAdapter->prAisBssInfo->ucBssIndex,
							    prCmd->aucPeerMac);
				prStaRec->ucTdlsIndex = i;
				break;
			}
		}
		break;
	case TDLS_DISABLE_LINK:

		prStaRec =
		    cnmGetTdlsPeerByAddress(prAdapter, prAdapter->prAisBssInfo->ucBssIndex,
					    prCmd->aucPeerMac);
		g_arTdlsLink[prStaRec->ucTdlsIndex] = 0;
		if (IS_DLS_STA(prStaRec))
			cnmStaRecFree(prAdapter, prStaRec);
		break;
	default:
		return 0;
	}


	return 0;
}




/*----------------------------------------------------------------------------*/
/*!
* \brief This routine is called to append general IEs.
*
* \param[in] pvAdapter Pointer to the Adapter structure.
* \param[in]
*
* \retval append length
*/
/*----------------------------------------------------------------------------*/
UINT_32 TdlsFrameGeneralIeAppend(ADAPTER_T *prAdapter, STA_RECORD_T *prStaRec, UINT_8 *pPkt)
{
	GLUE_INFO_T *prGlueInfo;
	BSS_INFO_T *prBssInfo;
	PM_PROFILE_SETUP_INFO_T *prPmProfSetupInfo;
	UINT_32 u4NonHTPhyType;
	UINT_16 u2SupportedRateSet;
	UINT_8 aucAllSupportedRates[RATE_NUM_SW] = { 0 };	/* 6628 RATE_NUM -> 6630 RATE_NUM_SW */
	UINT_8 ucAllSupportedRatesLen;
	UINT_8 ucSupRatesLen;
	UINT_8 ucExtSupRatesLen;
	UINT_32 u4PktLen, u4IeLen;




	/* init */
	prGlueInfo = (GLUE_INFO_T *) prAdapter->prGlueInfo;
	prBssInfo = prAdapter->prAisBssInfo;	/* AIS only */

	prPmProfSetupInfo = &prBssInfo->rPmProfSetupInfo;
	u4PktLen = 0;

	/* 3. Frame Formation - (5) Supported Rates element */
	/* use all sup rate we can support */
	u4NonHTPhyType = prStaRec->ucNonHTBasicPhyType;
	u2SupportedRateSet = rNonHTPhyAttributes[u4NonHTPhyType].u2SupportedRateSet;
	rateGetDataRatesFromRateSet(u2SupportedRateSet,
				    0, aucAllSupportedRates, &ucAllSupportedRatesLen);

	ucSupRatesLen = ((ucAllSupportedRatesLen > ELEM_MAX_LEN_SUP_RATES) ?
			 ELEM_MAX_LEN_SUP_RATES : ucAllSupportedRatesLen);


	ucExtSupRatesLen = ucAllSupportedRatesLen - ucSupRatesLen;

	if (ucSupRatesLen) {
		SUP_RATES_IE(pPkt)->ucId = ELEM_ID_SUP_RATES;
		SUP_RATES_IE(pPkt)->ucLength = ucSupRatesLen;
		kalMemCopy(SUP_RATES_IE(pPkt)->aucSupportedRates,
			   aucAllSupportedRates, ucSupRatesLen);

		u4IeLen = IE_SIZE(pPkt);
		pPkt += u4IeLen;
		u4PktLen += u4IeLen;
	}

	/* 3. Frame Formation - (7) Extended sup rates element */
	if (ucExtSupRatesLen) {

		EXT_SUP_RATES_IE(pPkt)->ucId = ELEM_ID_EXTENDED_SUP_RATES;
		EXT_SUP_RATES_IE(pPkt)->ucLength = ucExtSupRatesLen;

		kalMemCopy(EXT_SUP_RATES_IE(pPkt)->aucExtSupportedRates,
			   &aucAllSupportedRates[ucSupRatesLen], ucExtSupRatesLen);

		u4IeLen = IE_SIZE(pPkt);
		pPkt += u4IeLen;
		u4PktLen += u4IeLen;
	}

	/* 3. Frame Formation - (8) Supported channels element */
	SUPPORTED_CHANNELS_IE(pPkt)->ucId = ELEM_ID_SUP_CHS;
	SUPPORTED_CHANNELS_IE(pPkt)->ucLength = 2;
	SUPPORTED_CHANNELS_IE(pPkt)->ucChannelNum[0] = 1;
	SUPPORTED_CHANNELS_IE(pPkt)->ucChannelNum[1] = 13;

	if (prAdapter->fgEnable5GBand == TRUE) {
		SUPPORTED_CHANNELS_IE(pPkt)->ucLength = 10;
		SUPPORTED_CHANNELS_IE(pPkt)->ucChannelNum[2] = 36;
		SUPPORTED_CHANNELS_IE(pPkt)->ucChannelNum[3] = 4;
		SUPPORTED_CHANNELS_IE(pPkt)->ucChannelNum[4] = 52;
		SUPPORTED_CHANNELS_IE(pPkt)->ucChannelNum[5] = 4;
		SUPPORTED_CHANNELS_IE(pPkt)->ucChannelNum[6] = 149;
		SUPPORTED_CHANNELS_IE(pPkt)->ucChannelNum[7] = 4;
		SUPPORTED_CHANNELS_IE(pPkt)->ucChannelNum[8] = 165;
		SUPPORTED_CHANNELS_IE(pPkt)->ucChannelNum[9] = 1;
	}


	u4IeLen = IE_SIZE(pPkt);
	pPkt += u4IeLen;
	u4PktLen += u4IeLen;

	return u4PktLen;
}


 /*******************************************************************************
 *						 P U B L I C  F U N C T I O N S
 ********************************************************************************
 */

/*!
* \brief This routine is called to transmit a TDLS data frame.
*
* \param[in] pvAdapter Pointer to the Adapter structure.
* \param[in]
* \param[in]
* \param[in] buf includes RSN IE + FT IE + Lifetimeout IE
*
* \retval WLAN_STATUS_SUCCESS
* \retval WLAN_STATUS_INVALID_LENGTH
*/
/*----------------------------------------------------------------------------*/
WLAN_STATUS
TdlsDataFrameSend_TearDown(ADAPTER_T *prAdapter,
			   STA_RECORD_T *prStaRec,
			   UINT_8 *pPeerMac,
			   UINT_8 ucActionCode,
			   UINT_8 ucDialogToken,
			   UINT_16 u2StatusCode, UINT_8 *pAppendIe, UINT_32 AppendIeLen)
{

	GLUE_INFO_T *prGlueInfo;
	BSS_INFO_T *prBssInfo;
	PM_PROFILE_SETUP_INFO_T *prPmProfSetupInfo;
	struct sk_buff *prMsduInfo;
	UINT_8 *pPkt;
	UINT_32 u4PktLen, u4IeLen;
	UINT_8 ReasonCode;


	/* allocate/init packet */
	prGlueInfo = (GLUE_INFO_T *) prAdapter->prGlueInfo;
	prBssInfo = prAdapter->prAisBssInfo;	/* AIS only */


	prPmProfSetupInfo = &prBssInfo->rPmProfSetupInfo;
	u4PktLen = 0;

	prMsduInfo = kalPacketAlloc(prGlueInfo, 1600, &pPkt);
	if (prMsduInfo == NULL) {
		return TDLS_STATUS_RESOURCES;
	}

	prMsduInfo->dev = prGlueInfo->prDevHandler;
	if (prMsduInfo->dev == NULL) {
		kalPacketFree(prGlueInfo, prMsduInfo);
		return TDLS_STATUS_FAIL;
	}

	/* make up frame content */
	/* 1. 802.3 header */
	kalMemCopy(pPkt, pPeerMac, TDLS_FME_MAC_ADDR_LEN);
	pPkt += TDLS_FME_MAC_ADDR_LEN;
	kalMemCopy(pPkt, prAdapter->rMyMacAddr, TDLS_FME_MAC_ADDR_LEN);
	pPkt += TDLS_FME_MAC_ADDR_LEN;
	*(UINT_16 *) pPkt = htons(TDLS_FRM_PROT_TYPE);
	pPkt += 2;
	u4PktLen += TDLS_FME_MAC_ADDR_LEN * 2 + 2;

	/* 2. payload type */
	*pPkt = TDLS_FRM_PAYLOAD_TYPE;
	pPkt++;
	u4PktLen++;

	/* 3. Frame Formation - (1) Category */
	*pPkt = TDLS_FRM_CATEGORY;
	pPkt++;
	u4PktLen++;

	/* 3. Frame Formation - (2) Action */
	*pPkt = ucActionCode;
	pPkt++;
	u4PktLen++;

	/* 3. Frame Formation - status code */

	ReasonCode = u2StatusCode;
	kalMemCopy(pPkt, &ReasonCode, 2);
	pPkt = pPkt + 2;
	u4PktLen = u4PktLen + 2;


	/* 3. Frame Formation - (16) Link identifier element */
	TDLS_LINK_IDENTIFIER_IE(pPkt)->ucId = ELEM_ID_LINK_IDENTIFIER;
	TDLS_LINK_IDENTIFIER_IE(pPkt)->ucLength = 18;

	kalMemCopy(TDLS_LINK_IDENTIFIER_IE(pPkt)->aBSSID, prBssInfo->aucBSSID, 6);
	kalMemCopy(TDLS_LINK_IDENTIFIER_IE(pPkt)->aInitiator, prAdapter->rMyMacAddr, 6);
	kalMemCopy(TDLS_LINK_IDENTIFIER_IE(pPkt)->aResponder, pPeerMac, 6);

	u4IeLen = IE_SIZE(pPkt);
	pPkt += u4IeLen;
	u4PktLen += u4IeLen;

	/* 5. Update packet length */
	prMsduInfo->len = u4PktLen;

	/* 5. send the data frame */
	wlanHardStartXmit(prMsduInfo, prMsduInfo->dev);

	return TDLS_STATUS_SUCCESS;
}


/*!
* \brief This routine is called to transmit a TDLS data frame.
*
* \param[in] pvAdapter Pointer to the Adapter structure.
* \param[in]
* \param[in]
* \param[in] buf includes RSN IE + FT IE + Lifetimeout IE
*
* \retval WLAN_STATUS_SUCCESS
* \retval WLAN_STATUS_INVALID_LENGTH
*/
/*----------------------------------------------------------------------------*/
WLAN_STATUS			/* TDLS_STATUS */
TdlsDataFrameSend_SETUP_REQ(ADAPTER_T *prAdapter,
			    STA_RECORD_T *prStaRec,
			    UINT_8 *pPeerMac,
			    UINT_8 ucActionCode,
			    UINT_8 ucDialogToken,
			    UINT_16 u2StatusCode, UINT_8 *pAppendIe, UINT_32 AppendIeLen)
{


	GLUE_INFO_T *prGlueInfo;
	BSS_INFO_T *prBssInfo;
	PM_PROFILE_SETUP_INFO_T *prPmProfSetupInfo;
	struct sk_buff *prMsduInfo;
	UINT_8 *pPkt;
	UINT_32 u4PktLen, u4IeLen;
	BOOLEAN fg40mAllowed;
	UINT_16 u2CapInfo;



	/* allocate/init packet */
	prGlueInfo = (GLUE_INFO_T *) prAdapter->prGlueInfo;
	prBssInfo = prAdapter->prAisBssInfo;	/* AIS only */



	prPmProfSetupInfo = &prBssInfo->rPmProfSetupInfo;
	u4PktLen = 0;

	prMsduInfo = kalPacketAlloc(prGlueInfo, 1600, &pPkt);
	if (prMsduInfo == NULL) {
		return TDLS_STATUS_RESOURCES;
	}

	prMsduInfo->dev = prGlueInfo->prDevHandler;
	if (prMsduInfo->dev == NULL) {
		kalPacketFree(prGlueInfo, prMsduInfo);
		return TDLS_STATUS_FAIL;
	}

	/* make up frame content */
	/* 1. 802.3 header */
	kalMemCopy(pPkt, pPeerMac, TDLS_FME_MAC_ADDR_LEN);
	pPkt += TDLS_FME_MAC_ADDR_LEN;
	kalMemCopy(pPkt, prAdapter->rMyMacAddr, TDLS_FME_MAC_ADDR_LEN);
	pPkt += TDLS_FME_MAC_ADDR_LEN;
	*(UINT_16 *) pPkt = htons(TDLS_FRM_PROT_TYPE);
	pPkt += 2;
	u4PktLen += TDLS_FME_MAC_ADDR_LEN * 2 + 2;

	/* 2. payload type */
	*pPkt = TDLS_FRM_PAYLOAD_TYPE;
	pPkt++;
	u4PktLen++;

	/* 3. Frame Formation - (1) Category */
	*pPkt = TDLS_FRM_CATEGORY;
	pPkt++;
	u4PktLen++;

	/* 3. Frame Formation - (2) Action */
	*pPkt = ucActionCode;
	pPkt++;
	u4PktLen++;

	/* 3. Frame Formation - (3) Dialog token */
	*pPkt = ucDialogToken;
	pPkt++;
	u4PktLen++;

	/* 3. Frame Formation - (4) Capability */
	u2CapInfo = assocBuildCapabilityInfo(prAdapter, prStaRec);
	WLAN_SET_FIELD_16(pPkt, u2CapInfo);
	pPkt = pPkt + 2;
	u4PktLen = u4PktLen + 2;

	/* 4. Append general IEs */
	u4IeLen = TdlsFrameGeneralIeAppend(prAdapter, prStaRec, pPkt);
	pPkt += u4IeLen;
	u4PktLen += u4IeLen;

	/* 4. Append extra IEs */
	kalMemCopy(pPkt, pAppendIe, AppendIeLen);
	pPkt += AppendIeLen;
	u4PktLen += AppendIeLen;

	/* 3. Frame Formation - (10) Extended capabilities element */
	EXT_CAP_IE(pPkt)->ucId = ELEM_ID_EXTENDED_CAP;
	EXT_CAP_IE(pPkt)->ucLength = 5;
/* 0320 !! */
	EXT_CAP_IE(pPkt)->aucCapabilities[0] = 0x00;	/* bit0 ~ bit7 */
	EXT_CAP_IE(pPkt)->aucCapabilities[1] = 0x00;	/* bit8 ~ bit15 */
	EXT_CAP_IE(pPkt)->aucCapabilities[2] = 0x00;	/* bit16 ~ bit23 */
	EXT_CAP_IE(pPkt)->aucCapabilities[3] = 0x00;	/* bit24 ~ bit31 */
	EXT_CAP_IE(pPkt)->aucCapabilities[4] = 0xFF;	/* bit32 ~ bit39 */


	EXT_CAP_IE(pPkt)->aucCapabilities[3] |= BIT((28 - 24));
	EXT_CAP_IE(pPkt)->aucCapabilities[3] |= BIT((30 - 24));
	EXT_CAP_IE(pPkt)->aucCapabilities[4] |= BIT((37 - 32));

	/* EXT_CAP_IE(pPkt)->aucCapabilities[3] = 0x00; */ /* bit24 ~ bit31 */

	u4IeLen = IE_SIZE(pPkt);
	pPkt += u4IeLen;
	u4PktLen += u4IeLen;


	/* HT capability IE append 0122 */
	HT_CAP_IE(pPkt)->ucId = ELEM_ID_HT_CAP;
	HT_CAP_IE(pPkt)->ucLength = 26;


	/* 3. Frame Formation - (14) HT capabilities element */
	if ((prAdapter->rWifiVar.ucAvailablePhyTypeSet & PHY_TYPE_SET_802_11N)) {
		/* TODO: prAdapter->rWifiVar.rConnSettings.uc5GBandwidthMode */
		if (prAdapter->rWifiVar.rConnSettings.uc2G4BandwidthMode == CONFIG_BW_20M)
			fg40mAllowed = FALSE;
		else
			fg40mAllowed = TRUE;

		/* Add HT IE */ /*try to reuse p2p path */
		u4IeLen = rlmFillHtCapIEByAdapter(prAdapter, prBssInfo, pPkt);
		pPkt += u4IeLen;
		u4PktLen += u4IeLen;
		/* 0320 !! check newest driver !!! */
	}
/* check */

	/* 3. Frame Formation - (12) Timeout interval element (TPK Key Lifetime) */
	TIMEOUT_INTERVAL_IE(pPkt)->ucId = ELEM_ID_TIMEOUT_INTERVAL;
	TIMEOUT_INTERVAL_IE(pPkt)->ucLength = 5;

	TIMEOUT_INTERVAL_IE(pPkt)->ucType = 2;	/* IE_TIMEOUT_INTERVAL_TYPE_KEY_LIFETIME; */
	TIMEOUT_INTERVAL_IE(pPkt)->u4Value = ELEM_ID_TIMEOUT_INTERVAL;	/* htonl(prCmd->u4Timeout); */

	u4IeLen = IE_SIZE(pPkt);
	pPkt += u4IeLen;
	u4PktLen += u4IeLen;


	/* 3. Frame Formation - (16) Link identifier element */
	TDLS_LINK_IDENTIFIER_IE(pPkt)->ucId = ELEM_ID_TIMEOUT_INTERVAL;
	TDLS_LINK_IDENTIFIER_IE(pPkt)->ucLength = 18;

	kalMemCopy(TDLS_LINK_IDENTIFIER_IE(pPkt)->aBSSID, prBssInfo->aucBSSID, 6);
	kalMemCopy(TDLS_LINK_IDENTIFIER_IE(pPkt)->aInitiator, prAdapter->rMyMacAddr, 6);
	kalMemCopy(TDLS_LINK_IDENTIFIER_IE(pPkt)->aResponder, pPeerMac, 6);

	u4IeLen = IE_SIZE(pPkt);
	pPkt += u4IeLen;
	u4PktLen += u4IeLen;





	/* 3. Frame Formation - (17) WMM Information element */


	if (IS_FEATURE_ENABLED(prAdapter->rWifiVar.ucQoS)) {
#if 0
		u4IeLen = mqmGenerateWmmInfoIEByParam(prAdapter->rWifiVar.fgSupportUAPSD,
						      prPmProfSetupInfo->ucBmpDeliveryAC,
						      prPmProfSetupInfo->ucBmpTriggerAC,
						      prPmProfSetupInfo->ucUapsdSp, pPkt);
#endif

		/* Add WMM IE */ /* try to reuse p2p path */
		u4IeLen = mqmGenerateWmmInfoIEByStaRec(prAdapter, prBssInfo, prStaRec, pPkt);
		pPkt += u4IeLen;
		u4PktLen += u4IeLen;
	}
#if CFG_SUPPORT_802_11AC
	if (prAdapter->rWifiVar.ucAvailablePhyTypeSet & PHY_TYPE_SET_802_11AC) {
		/* Add VHT IE */ /* try to reuse p2p path */
		u4IeLen = rlmFillVhtCapIEByAdapter(prAdapter, prBssInfo, pPkt);
		pPkt += u4IeLen;
		u4PktLen += u4IeLen;
	}
#endif

	/* 5. Update packet length */
	prMsduInfo->len = u4PktLen;

	/* 5. send the data frame */
	wlanHardStartXmit(prMsduInfo, prMsduInfo->dev);
	/* wlanTx ??? */
	return TDLS_STATUS_SUCCESS;
}

WLAN_STATUS
TdlsDataFrameSend_SETUP_RSP(ADAPTER_T *prAdapter,
			    STA_RECORD_T *prStaRec,
			    UINT_8 *pPeerMac,
			    UINT_8 ucActionCode,
			    UINT_8 ucDialogToken,
			    UINT_16 u2StatusCode, UINT_8 *pAppendIe, UINT_32 AppendIeLen)
{

	GLUE_INFO_T *prGlueInfo;
	BSS_INFO_T *prBssInfo;
	PM_PROFILE_SETUP_INFO_T *prPmProfSetupInfo;
	struct sk_buff *prMsduInfo;
	UINT_8 *pPkt;
	UINT_32 u4PktLen, u4IeLen;
	UINT_16 u2CapInfo;
	UINT_16 StatusCode;
	BOOLEAN fg40mAllowed;


	/* allocate/init packet */
	prGlueInfo = (GLUE_INFO_T *) prAdapter->prGlueInfo;
	prBssInfo = prAdapter->prAisBssInfo;	/* AIS only */
	prPmProfSetupInfo = &prBssInfo->rPmProfSetupInfo;
	u4PktLen = 0;

	prMsduInfo = kalPacketAlloc(prGlueInfo, 1600, &pPkt);
	if (prMsduInfo == NULL) {
		return TDLS_STATUS_RESOURCES;
	}

	prMsduInfo->dev = prGlueInfo->prDevHandler;
	if (prMsduInfo->dev == NULL) {
		kalPacketFree(prGlueInfo, prMsduInfo);
		return TDLS_STATUS_FAIL;
	}

	/* make up frame content */
	/* 1. 802.3 header */
	kalMemCopy(pPkt, pPeerMac, TDLS_FME_MAC_ADDR_LEN);
	pPkt += TDLS_FME_MAC_ADDR_LEN;
	kalMemCopy(pPkt, prAdapter->rMyMacAddr, TDLS_FME_MAC_ADDR_LEN);
	pPkt += TDLS_FME_MAC_ADDR_LEN;
	*(UINT_16 *) pPkt = htons(TDLS_FRM_PROT_TYPE);
	pPkt += 2;
	u4PktLen += TDLS_FME_MAC_ADDR_LEN * 2 + 2;

	/* 2. payload type */
	*pPkt = TDLS_FRM_PAYLOAD_TYPE;
	pPkt++;
	u4PktLen++;

	/* 3. Frame Formation - (1) Category */
	*pPkt = TDLS_FRM_CATEGORY;
	pPkt++;
	u4PktLen++;

	/* 3. Frame Formation - (2) Action */
	*pPkt = ucActionCode;
	pPkt++;
	u4PktLen++;


	/* 3. Frame Formation - status code */
	StatusCode = u2StatusCode;
	kalMemCopy(pPkt, &StatusCode, 2);
	pPkt = pPkt + 2;
	u4PktLen = u4PktLen + 2;



	/* 3. Frame Formation - (3) Dialog token */
	*pPkt = ucDialogToken;
	pPkt++;
	u4PktLen++;

	/* 3. Frame Formation - (4) Capability */
	u2CapInfo = assocBuildCapabilityInfo(prAdapter, prStaRec);
	WLAN_SET_FIELD_16(pPkt, u2CapInfo);
	pPkt = pPkt + 2;
	u4PktLen = u4PktLen + 2;


	/* 4. Append general IEs */
	u4IeLen = TdlsFrameGeneralIeAppend(prAdapter, prStaRec, pPkt);
	pPkt += u4IeLen;
	u4PktLen += u4IeLen;

	/* 4. Append extra IEs */
	kalMemCopy(pPkt, pAppendIe, AppendIeLen);
	pPkt += AppendIeLen;
	u4PktLen += AppendIeLen;

	/* 3. Frame Formation - (10) Extended capabilities element */
	EXT_CAP_IE(pPkt)->ucId = ELEM_ID_EXTENDED_CAP;
	EXT_CAP_IE(pPkt)->ucLength = 5;

	EXT_CAP_IE(pPkt)->aucCapabilities[0] = 0x00;	/* bit0 ~ bit7 */
	EXT_CAP_IE(pPkt)->aucCapabilities[1] = 0x00;	/* bit8 ~ bit15 */
	EXT_CAP_IE(pPkt)->aucCapabilities[2] = 0x00;	/* bit16 ~ bit23 */
	EXT_CAP_IE(pPkt)->aucCapabilities[3] = 0x00;	/* bit24 ~ bit31 */
	EXT_CAP_IE(pPkt)->aucCapabilities[4] = 0xFF;	/* bit32 ~ bit39 */

	EXT_CAP_IE(pPkt)->aucCapabilities[3] |= BIT((28 - 24));

	EXT_CAP_IE(pPkt)->aucCapabilities[3] |= BIT((30 - 24));
	EXT_CAP_IE(pPkt)->aucCapabilities[4] |= BIT((37 - 32));
	/* EXT_CAP_IE(pPkt)->aucCapabilities[3] = 0x00; */ /* bit24 ~ bit31 */
	u4IeLen = IE_SIZE(pPkt);
	pPkt += u4IeLen;
	u4PktLen += u4IeLen;

	/* HT capability IE append */
	HT_CAP_IE(pPkt)->ucId = ELEM_ID_HT_CAP;
	HT_CAP_IE(pPkt)->ucLength = 26;


	/* 3. Frame Formation - (14) HT capabilities element */
	if ((prAdapter->rWifiVar.ucAvailablePhyTypeSet & PHY_TYPE_SET_802_11N)) {
		/* TODO: prAdapter->rWifiVar.rConnSettings.uc5GBandwidthMode */
		if (prAdapter->rWifiVar.rConnSettings.uc2G4BandwidthMode == CONFIG_BW_20M)
			fg40mAllowed = FALSE;
		else
			fg40mAllowed = TRUE;

		/* Add HT IE */ /* try to reuse p2p path */
		u4IeLen = rlmFillHtCapIEByAdapter(prAdapter, prBssInfo, pPkt);
		pPkt += u4IeLen;
		u4PktLen += u4IeLen;
	}

	/* 3. Frame Formation - (12) Timeout interval element (TPK Key Lifetime) */
	TIMEOUT_INTERVAL_IE(pPkt)->ucId = ELEM_ID_TIMEOUT_INTERVAL;
	TIMEOUT_INTERVAL_IE(pPkt)->ucLength = 5;
	TIMEOUT_INTERVAL_IE(pPkt)->ucType = 2;	/* IE_TIMEOUT_INTERVAL_TYPE_KEY_LIFETIME; */
	TIMEOUT_INTERVAL_IE(pPkt)->u4Value = 43200;	/* htonl(prCmd->u4Timeout); */

	u4IeLen = IE_SIZE(pPkt);
	pPkt += u4IeLen;
	u4PktLen += u4IeLen;

	/* 3. Frame Formation - (16) Link identifier element */
	TDLS_LINK_IDENTIFIER_IE(pPkt)->ucId = ELEM_ID_LINK_IDENTIFIER;
	TDLS_LINK_IDENTIFIER_IE(pPkt)->ucLength = 18;

	kalMemCopy(TDLS_LINK_IDENTIFIER_IE(pPkt)->aBSSID, prBssInfo->aucBSSID, 6);
	kalMemCopy(TDLS_LINK_IDENTIFIER_IE(pPkt)->aInitiator, pPeerMac, 6);	/* prAdapter->rMyMacAddr */
	kalMemCopy(TDLS_LINK_IDENTIFIER_IE(pPkt)->aResponder, prAdapter->rMyMacAddr, 6);	/* pPeerMac */

	u4IeLen = IE_SIZE(pPkt);
	pPkt += u4IeLen;
	u4PktLen += u4IeLen;


	/* HT WMM IE append */
	if (IS_FEATURE_ENABLED(prAdapter->rWifiVar.ucQoS)) {

		/* Add WMM IE */ /* try to reuse p2p path */
		u4IeLen = mqmGenerateWmmInfoIEByStaRec(prAdapter, prBssInfo, prStaRec, pPkt);
		pPkt += u4IeLen;
		u4PktLen += u4IeLen;
	}
#if CFG_SUPPORT_802_11AC
	if (prAdapter->rWifiVar.ucAvailablePhyTypeSet & PHY_TYPE_SET_802_11AC) {
		/* Add VHT IE */ /* try to reuse p2p path */
		u4IeLen = rlmFillVhtCapIEByAdapter(prAdapter, prBssInfo, pPkt);
		pPkt += u4IeLen;
		u4PktLen += u4IeLen;
	}
#endif


	/* 5. Update packet length */
	prMsduInfo->len = u4PktLen;

	/* 5. send the data frame */
	wlanHardStartXmit(prMsduInfo, prMsduInfo->dev);

	return TDLS_STATUS_SUCCESS;
}


WLAN_STATUS
TdlsDataFrameSend_CONFIRM(ADAPTER_T *prAdapter,
			  STA_RECORD_T *prStaRec,
			  UINT_8 *pPeerMac,
			  UINT_8 ucActionCode,
			  UINT_8 ucDialogToken,
			  UINT_16 u2StatusCode, UINT_8 *pAppendIe, UINT_32 AppendIeLen)
{

	GLUE_INFO_T *prGlueInfo;
	BSS_INFO_T *prBssInfo;
	PM_PROFILE_SETUP_INFO_T *prPmProfSetupInfo;
	struct sk_buff *prMsduInfo;
	UINT_8 *pPkt;
	UINT_32 u4PktLen, u4IeLen;
	UINT_16 StatusCode;

	/* allocate/init packet */
	prGlueInfo = (GLUE_INFO_T *) prAdapter->prGlueInfo;
	prBssInfo = prAdapter->prAisBssInfo;	/* AIS only */

	prPmProfSetupInfo = &prBssInfo->rPmProfSetupInfo;
	u4PktLen = 0;

	prMsduInfo = kalPacketAlloc(prGlueInfo, 1600, &pPkt);
	if (prMsduInfo == NULL) {
		return TDLS_STATUS_RESOURCES;
	}

	prMsduInfo->dev = prGlueInfo->prDevHandler;
	if (prMsduInfo->dev == NULL) {
		kalPacketFree(prGlueInfo, prMsduInfo);
		return TDLS_STATUS_FAIL;
	}

	/* make up frame content */
	/* 1. 802.3 header */
	kalMemCopy(pPkt, pPeerMac, TDLS_FME_MAC_ADDR_LEN);
	pPkt += TDLS_FME_MAC_ADDR_LEN;
	kalMemCopy(pPkt, prAdapter->rMyMacAddr, TDLS_FME_MAC_ADDR_LEN);
	pPkt += TDLS_FME_MAC_ADDR_LEN;
	*(UINT_16 *) pPkt = htons(TDLS_FRM_PROT_TYPE);
	pPkt += 2;
	u4PktLen += TDLS_FME_MAC_ADDR_LEN * 2 + 2;

	/* 2. payload type */
	*pPkt = TDLS_FRM_PAYLOAD_TYPE;
	pPkt++;
	u4PktLen++;

	/* 3. Frame Formation - (1) Category */
	*pPkt = TDLS_FRM_CATEGORY;
	pPkt++;
	u4PktLen++;

	/* 3. Frame Formation - (2) Action */
	*pPkt = ucActionCode;
	pPkt++;
	u4PktLen++;

	/* 3. Frame Formation - status code */

	StatusCode = u2StatusCode;	/* 0;  //u2StatusCode;  //ahiu 0224 */
	kalMemCopy(pPkt, &StatusCode, 2);
	pPkt = pPkt + 2;
	u4PktLen = u4PktLen + 2;


	/* 3. Frame Formation - (3) Dialog token */
	*pPkt = ucDialogToken;
	pPkt++;
	u4PktLen++;

	/* 4. Append extra IEs */
	kalMemCopy(pPkt, pAppendIe, AppendIeLen);
	pPkt += AppendIeLen;
	u4PktLen += AppendIeLen;


	/* 3. Frame Formation - (12) Timeout interval element (TPK Key Lifetime) */
	TIMEOUT_INTERVAL_IE(pPkt)->ucId = ELEM_ID_TIMEOUT_INTERVAL;
	TIMEOUT_INTERVAL_IE(pPkt)->ucLength = 5;

	TIMEOUT_INTERVAL_IE(pPkt)->ucType = 2;
	TIMEOUT_INTERVAL_IE(pPkt)->u4Value = 43200;	/* htonl(prCmd->u4Timeout); */

	u4IeLen = IE_SIZE(pPkt);
	pPkt += u4IeLen;
	u4PktLen += u4IeLen;

	/* 3. Frame Formation - (16) Link identifier element */
	TDLS_LINK_IDENTIFIER_IE(pPkt)->ucId = ELEM_ID_LINK_IDENTIFIER;
	TDLS_LINK_IDENTIFIER_IE(pPkt)->ucLength = 18;

	kalMemCopy(TDLS_LINK_IDENTIFIER_IE(pPkt)->aBSSID, prBssInfo->aucBSSID, 6);
	kalMemCopy(TDLS_LINK_IDENTIFIER_IE(pPkt)->aInitiator, prAdapter->rMyMacAddr, 6);
	kalMemCopy(TDLS_LINK_IDENTIFIER_IE(pPkt)->aResponder, pPeerMac, 6);

	u4IeLen = IE_SIZE(pPkt);
	pPkt += u4IeLen;
	u4PktLen += u4IeLen;

	/* 5. Update packet length */
	prMsduInfo->len = u4PktLen;

	/* 5. send the data frame */
	wlanHardStartXmit(prMsduInfo, prMsduInfo->dev);

	return TDLS_STATUS_SUCCESS;
}



/*
* \brief This routine is called to transmit a TDLS data frame.
*
* \param[in] pvAdapter Pointer to the Adapter structure.
* \param[in]
* \param[in]
* \param[in] buf includes RSN IE + FT IE + Lifetimeout IE
*
* \retval WLAN_STATUS_SUCCESS
* \retval WLAN_STATUS_INVALID_LENGTH
*/
/*----------------------------------------------------------------------------*/
WLAN_STATUS			/* TDLS_STATUS */
TdlsDataFrameSend_DISCOVERY_REQ(ADAPTER_T *prAdapter,
				STA_RECORD_T *prStaRec,
				UINT_8 *pPeerMac,
				UINT_8 ucActionCode,
				UINT_8 ucDialogToken,
				UINT_16 u2StatusCode, UINT_8 *pAppendIe, UINT_32 AppendIeLen)
{
#define LR_TDLS_FME_FIELD_FILL(__Len) \
		pPkt += __Len; \
		u4PktLen += __Len;

	GLUE_INFO_T *prGlueInfo;
	BSS_INFO_T *prBssInfo;
	PM_PROFILE_SETUP_INFO_T *prPmProfSetupInfo;
	struct sk_buff *prMsduInfo;
	MSDU_INFO_T *prMsduInfoMgmt;
	UINT8 *pPkt, *pucInitiator, *pucResponder;
	UINT32 u4PktLen, u4IeLen;
	UINT16 u2CapInfo;


	prGlueInfo = (GLUE_INFO_T *) prAdapter->prGlueInfo;

	if (prStaRec != NULL) {
		prBssInfo = prAdapter->prAisBssInfo;	/* AIS only */

	} else {
		return TDLS_STATUS_FAIL;
	}



	/* allocate/init packet */
	prPmProfSetupInfo = &prBssInfo->rPmProfSetupInfo;
	u4PktLen = 0;
	prMsduInfo = NULL;
	prMsduInfoMgmt = NULL;

	/* make up frame content */
	if (ucActionCode != TDLS_FRM_ACTION_DISCOVERY_RSP) {
		/* TODO: reduce 1600 to correct size */
		prMsduInfo = kalPacketAlloc(prGlueInfo, 1600, &pPkt);
		if (prMsduInfo == NULL) {

			return TDLS_STATUS_RESOURCES;
		}

		prMsduInfo->dev = prGlueInfo->prDevHandler;
		if (prMsduInfo->dev == NULL) {

			kalPacketFree(prGlueInfo, prMsduInfo);
			return TDLS_STATUS_FAIL;
		}

		/* 1. 802.3 header */
		kalMemCopy(pPkt, pPeerMac, TDLS_FME_MAC_ADDR_LEN);
		LR_TDLS_FME_FIELD_FILL(TDLS_FME_MAC_ADDR_LEN);
		kalMemCopy(pPkt, prBssInfo->aucOwnMacAddr, TDLS_FME_MAC_ADDR_LEN);
		LR_TDLS_FME_FIELD_FILL(TDLS_FME_MAC_ADDR_LEN);
		*(UINT_16 *) pPkt = htons(TDLS_FRM_PROT_TYPE);
		LR_TDLS_FME_FIELD_FILL(2);

		/* 2. payload type */
		*pPkt = TDLS_FRM_PAYLOAD_TYPE;
		LR_TDLS_FME_FIELD_FILL(1);

		/* 3. Frame Formation - (1) Category */
		*pPkt = TDLS_FRM_CATEGORY;
		LR_TDLS_FME_FIELD_FILL(1);
	} else {
		WLAN_MAC_HEADER_T *prHdr;

		prMsduInfoMgmt = (MSDU_INFO_T *)
		    cnmMgtPktAlloc(prAdapter, PUBLIC_ACTION_MAX_LEN);
		if (prMsduInfoMgmt == NULL) {

			return TDLS_STATUS_RESOURCES;
		}

		pPkt = (UINT8 *) prMsduInfoMgmt->prPacket;
		prHdr = (WLAN_MAC_HEADER_T *) pPkt;

		/* 1. 802.11 header */
		prHdr->u2FrameCtrl = MAC_FRAME_ACTION;
		prHdr->u2DurationID = 0;
		kalMemCopy(prHdr->aucAddr1, pPeerMac, TDLS_FME_MAC_ADDR_LEN);
		kalMemCopy(prHdr->aucAddr2, prBssInfo->aucOwnMacAddr, TDLS_FME_MAC_ADDR_LEN);
		kalMemCopy(prHdr->aucAddr3, prBssInfo->aucBSSID, TDLS_FME_MAC_ADDR_LEN);
		prHdr->u2SeqCtrl = 0;
		LR_TDLS_FME_FIELD_FILL(sizeof(WLAN_MAC_HEADER_T));

		/* Frame Formation - (1) Category */
		*pPkt = CATEGORY_PUBLIC_ACTION;
		LR_TDLS_FME_FIELD_FILL(1);
	}

	/* 3. Frame Formation - (2) Action */
	*pPkt = ucActionCode;
	LR_TDLS_FME_FIELD_FILL(1);

	/* 3. Frame Formation - Status Code */
	switch (ucActionCode) {
	case TDLS_FRM_ACTION_SETUP_RSP:
	case TDLS_FRM_ACTION_CONFIRM:
	case TDLS_FRM_ACTION_TEARDOWN:
		WLAN_SET_FIELD_16(pPkt, u2StatusCode);
		LR_TDLS_FME_FIELD_FILL(2);
		break;
	}

	/* 3. Frame Formation - (3) Dialog token */
	if (ucActionCode != TDLS_FRM_ACTION_TEARDOWN) {
		*pPkt = ucDialogToken;
		LR_TDLS_FME_FIELD_FILL(1);
	}

	/* Fill elements */
	if (ucActionCode != TDLS_FRM_ACTION_TEARDOWN) {
		/*
		   Capability

		   Support Rates
		   Extended Support Rates
		   Supported Channels
		   HT Capabilities
		   WMM Information Element

		   Extended Capabilities
		   Link Identifier

		   RSNIE
		   FTIE
		   Timeout Interval
		 */
		if (ucActionCode != TDLS_FRM_ACTION_CONFIRM) {
			/* 3. Frame Formation - (4) Capability: 0x31 0x04, privacy bit will be set */
			u2CapInfo = assocBuildCapabilityInfo(prAdapter, prStaRec);
			WLAN_SET_FIELD_16(pPkt, u2CapInfo);
			LR_TDLS_FME_FIELD_FILL(2);

			/* 4. Append general IEs */
			/*
			   TODO check HT: prAdapter->rWifiVar.rConnSettings.uc2G4BandwidthMode
			   must be CONFIG_BW_20_40M.

			   TODO check HT: HT_CAP_INFO_40M_INTOLERANT must be clear if
			   Tdls 20/40 is enabled.
			 */
			u4IeLen = TdlsFrameGeneralIeAppend(prAdapter, prStaRec, pPkt);
			LR_TDLS_FME_FIELD_FILL(u4IeLen);

			/* 5. Frame Formation - Extended capabilities element */
			EXT_CAP_IE(pPkt)->ucId = ELEM_ID_EXTENDED_CAP;
			EXT_CAP_IE(pPkt)->ucLength = 5;

			EXT_CAP_IE(pPkt)->aucCapabilities[0] = 0x00;	/* bit0 ~ bit7 */
			EXT_CAP_IE(pPkt)->aucCapabilities[1] = 0x00;	/* bit8 ~ bit15 */
			EXT_CAP_IE(pPkt)->aucCapabilities[2] = 0x00;	/* bit16 ~ bit23 */
			EXT_CAP_IE(pPkt)->aucCapabilities[3] = 0x00;	/* bit24 ~ bit31 */
			EXT_CAP_IE(pPkt)->aucCapabilities[4] = 0x00;	/* bit32 ~ bit39 */

			/* if (prCmd->ucExCap & TDLS_EX_CAP_PEER_UAPSD) */
			EXT_CAP_IE(pPkt)->aucCapabilities[3] |= BIT((28 - 24));
			/* if (prCmd->ucExCap & TDLS_EX_CAP_CHAN_SWITCH) */
			EXT_CAP_IE(pPkt)->aucCapabilities[3] |= BIT((30 - 24));
			/* if (prCmd->ucExCap & TDLS_EX_CAP_TDLS) */
			EXT_CAP_IE(pPkt)->aucCapabilities[4] |= BIT((37 - 32));

			u4IeLen = IE_SIZE(pPkt);
			LR_TDLS_FME_FIELD_FILL(u4IeLen);
		} else {
			/* 5. Frame Formation - WMM Information element */
			if (IS_FEATURE_ENABLED(prAdapter->rWifiVar.ucQoS)) {

				/* Add WMM IE */ /* try to reuse p2p path */
				u4IeLen =
				    mqmGenerateWmmInfoIEByStaRec(prAdapter, prBssInfo, prStaRec,
								 pPkt);
				pPkt += u4IeLen;
				u4PktLen += u4IeLen;
			}

		}
	}

	/* 6. Frame Formation - 20/40 BSS Coexistence */
	/*
	   Follow WiFi test plan, add 20/40 element to request/response/confirm.
	 */
#if 0
	if (prGlueInfo->fgTdlsIs2040Supported == TRUE) {
		/*
		   bit0 = 1: The Information Request field is used to indicate that a
		   transmitting STA is requesting the recipient to transmit a 20/40 BSS
		   Coexistence Management frame with the transmitting STA as the
		   recipient.

		   bit1 = 0: The Forty MHz Intolerant field is set to 1 to prohibit an AP
		   that receives this information or reports of this information from
		   operating a 20/40 MHz BSS.

		   bit2 = 0: The 20 MHz BSS Width Request field is set to 1 to prohibit
		   a receiving AP from operating its BSS as a 20/40 MHz BSS.
		 */
		BSS_20_40_COEXIST_IE(pPkt)->ucId = ELEM_ID_20_40_BSS_COEXISTENCE;
		BSS_20_40_COEXIST_IE(pPkt)->ucLength = 1;
		BSS_20_40_COEXIST_IE(pPkt)->ucData = 0x01;
		LR_TDLS_FME_FIELD_FILL(3);
	}
#endif
	/* 6. Frame Formation - HT Operation element */
	/* u4IeLen = rlmFillHtOpIeBody(prBssInfo, pPkt); */
	/* LR_TDLS_FME_FIELD_FILL(u4IeLen); */

	/* 7. Frame Formation - Link identifier element */
	/* Note: Link ID sequence must be correct; Or the calculated MIC will be error */
	TDLS_LINK_IDENTIFIER_IE(pPkt)->ucId = ELEM_ID_LINK_IDENTIFIER;
	TDLS_LINK_IDENTIFIER_IE(pPkt)->ucLength = 18;

	kalMemCopy(TDLS_LINK_IDENTIFIER_IE(pPkt)->aBSSID, prBssInfo->aucBSSID, 6);

	switch (ucActionCode) {
	case TDLS_FRM_ACTION_SETUP_REQ:
	case TDLS_FRM_ACTION_CONFIRM:
	case TDLS_FRM_ACTION_DISCOVERY_RSP:
	default:
		/* we are initiator */
		pucInitiator = prBssInfo->aucOwnMacAddr;
		pucResponder = pPeerMac;

		if (prStaRec != NULL)
			prStaRec->flgTdlsIsInitiator = TRUE;
		break;

	case TDLS_FRM_ACTION_SETUP_RSP:
		/* peer is initiator */
		pucInitiator = pPeerMac;
		pucResponder = prBssInfo->aucOwnMacAddr;

		if (prStaRec != NULL)
			prStaRec->flgTdlsIsInitiator = FALSE;
		break;

	case TDLS_FRM_ACTION_TEARDOWN:
		if (prStaRec != NULL) {
			if (prStaRec->flgTdlsIsInitiator == TRUE) {
				/* we are initiator */
				pucInitiator = prBssInfo->aucOwnMacAddr;
				pucResponder = pPeerMac;
			} else {
				/* peer is initiator */
				pucInitiator = pPeerMac;
				pucResponder = prBssInfo->aucOwnMacAddr;
			}
		} else {
			/* peer is initiator */
			pucInitiator = pPeerMac;
			pucResponder = prBssInfo->aucOwnMacAddr;
		}
		break;
	}

	kalMemCopy(TDLS_LINK_IDENTIFIER_IE(pPkt)->aInitiator, pucInitiator, 6);
	kalMemCopy(TDLS_LINK_IDENTIFIER_IE(pPkt)->aResponder, pucResponder, 6);

	u4IeLen = IE_SIZE(pPkt);
	LR_TDLS_FME_FIELD_FILL(u4IeLen);

	/* 8. Append security IEs */
	if ((ucActionCode != TDLS_FRM_ACTION_TEARDOWN) && (pAppendIe != NULL)) {
		kalMemCopy(pPkt, pAppendIe, AppendIeLen);
		LR_TDLS_FME_FIELD_FILL(AppendIeLen);
	}

	/* 10. send the data or management frame */
	if (ucActionCode != TDLS_FRM_ACTION_DISCOVERY_RSP) {
		/* 9. Update packet length */
		prMsduInfo->len = u4PktLen;

		wlanHardStartXmit(prMsduInfo, prMsduInfo->dev);
	} else {
		prMsduInfoMgmt->ucPacketType = TX_PACKET_TYPE_MGMT;
		prMsduInfoMgmt->ucStaRecIndex = prBssInfo->prStaRecOfAP->ucIndex;
		prMsduInfoMgmt->ucBssIndex = prBssInfo->ucBssIndex;
		prMsduInfoMgmt->ucMacHeaderLength = WLAN_MAC_MGMT_HEADER_LEN;
		prMsduInfoMgmt->fgIs802_1x = FALSE;
		prMsduInfoMgmt->fgIs802_11 = TRUE;
		prMsduInfoMgmt->u2FrameLength = u4PktLen;
		prMsduInfoMgmt->ucTxSeqNum = nicIncreaseTxSeqNum(prAdapter);
		prMsduInfoMgmt->pfTxDoneHandler = NULL;


		/* Send them to HW queue */
		nicTxEnqueueMsdu(prAdapter, prMsduInfoMgmt);
	}

	return TDLS_STATUS_SUCCESS;
}




WLAN_STATUS
TdlsDataFrameSend_DISCOVERY_RSP(ADAPTER_T *prAdapter,
				STA_RECORD_T *prStaRec,
				UINT_8 *pPeerMac,
				UINT_8 ucActionCode,
				UINT_8 ucDialogToken,
				UINT_16 u2StatusCode, UINT_8 *pAppendIe, UINT_32 AppendIeLen)
{
#define LR_TDLS_FME_FIELD_FILL(__Len) \
		pPkt += __Len; \
		u4PktLen += __Len;

	GLUE_INFO_T *prGlueInfo;
	BSS_INFO_T *prBssInfo;
	PM_PROFILE_SETUP_INFO_T *prPmProfSetupInfo;
	struct sk_buff *prMsduInfo;
	MSDU_INFO_T *prMsduInfoMgmt;
	UINT8 *pPkt, *pucInitiator, *pucResponder;
	UINT32 u4PktLen, u4IeLen;
	UINT16 u2CapInfo;


	prGlueInfo = (GLUE_INFO_T *) prAdapter->prGlueInfo;


	/* sanity check */
	if (prStaRec != NULL) {
		prBssInfo = prAdapter->prAisBssInfo;	/* AIS only */

	} else {
		return TDLS_STATUS_FAIL;
	}


	/* allocate/init packet */
	prPmProfSetupInfo = &prBssInfo->rPmProfSetupInfo;
	u4PktLen = 0;
	prMsduInfo = NULL;
	prMsduInfoMgmt = NULL;

	/* make up frame content */
	if (ucActionCode != TDLS_FRM_ACTION_DISCOVERY_RSP) {
		/* TODO: reduce 1600 to correct size */
		prMsduInfo = kalPacketAlloc(prGlueInfo, 1600, &pPkt);
		if (prMsduInfo == NULL) {

			return TDLS_STATUS_RESOURCES;
		}

		prMsduInfo->dev = prGlueInfo->prDevHandler;
		if (prMsduInfo->dev == NULL) {

			kalPacketFree(prGlueInfo, prMsduInfo);
			return TDLS_STATUS_FAIL;
		}

		/* 1. 802.3 header */
		kalMemCopy(pPkt, pPeerMac, TDLS_FME_MAC_ADDR_LEN);
		LR_TDLS_FME_FIELD_FILL(TDLS_FME_MAC_ADDR_LEN);
		kalMemCopy(pPkt, prBssInfo->aucOwnMacAddr, TDLS_FME_MAC_ADDR_LEN);
		LR_TDLS_FME_FIELD_FILL(TDLS_FME_MAC_ADDR_LEN);
		*(UINT_16 *) pPkt = htons(TDLS_FRM_PROT_TYPE);
		LR_TDLS_FME_FIELD_FILL(2);

		/* 2. payload type */
		*pPkt = TDLS_FRM_PAYLOAD_TYPE;
		LR_TDLS_FME_FIELD_FILL(1);

		/* 3. Frame Formation - (1) Category */
		*pPkt = TDLS_FRM_CATEGORY;
		LR_TDLS_FME_FIELD_FILL(1);
	} else {
		WLAN_MAC_HEADER_T *prHdr;

		prMsduInfoMgmt = (MSDU_INFO_T *)
		    cnmMgtPktAlloc(prAdapter, PUBLIC_ACTION_MAX_LEN);
		if (prMsduInfoMgmt == NULL) {

			return TDLS_STATUS_RESOURCES;
		}

		pPkt = (UINT8 *) prMsduInfoMgmt->prPacket;
		prHdr = (WLAN_MAC_HEADER_T *) pPkt;

		/* 1. 802.11 header */
		prHdr->u2FrameCtrl = MAC_FRAME_ACTION;
		prHdr->u2DurationID = 0;
		kalMemCopy(prHdr->aucAddr1, pPeerMac, TDLS_FME_MAC_ADDR_LEN);
		kalMemCopy(prHdr->aucAddr2, prBssInfo->aucOwnMacAddr, TDLS_FME_MAC_ADDR_LEN);
		kalMemCopy(prHdr->aucAddr3, prBssInfo->aucBSSID, TDLS_FME_MAC_ADDR_LEN);
		prHdr->u2SeqCtrl = 0;
		LR_TDLS_FME_FIELD_FILL(sizeof(WLAN_MAC_HEADER_T));

		/* Frame Formation - (1) Category */
		*pPkt = CATEGORY_PUBLIC_ACTION;
		LR_TDLS_FME_FIELD_FILL(1);
	}

	/* 3. Frame Formation - (2) Action */
	*pPkt = ucActionCode;
	LR_TDLS_FME_FIELD_FILL(1);

	/* 3. Frame Formation - Status Code */
	switch (ucActionCode) {
	case TDLS_FRM_ACTION_SETUP_RSP:
	case TDLS_FRM_ACTION_CONFIRM:
	case TDLS_FRM_ACTION_TEARDOWN:
		WLAN_SET_FIELD_16(pPkt, u2StatusCode);
		LR_TDLS_FME_FIELD_FILL(2);
		break;
	}

	/* 3. Frame Formation - (3) Dialog token */
	if (ucActionCode != TDLS_FRM_ACTION_TEARDOWN) {
		*pPkt = ucDialogToken;
		LR_TDLS_FME_FIELD_FILL(1);
	}

	/* Fill elements */
	if (ucActionCode != TDLS_FRM_ACTION_TEARDOWN) {
		/*
		   Capability

		   Support Rates
		   Extended Support Rates
		   Supported Channels
		   HT Capabilities
		   WMM Information Element

		   Extended Capabilities
		   Link Identifier

		   RSNIE
		   FTIE
		   Timeout Interval
		 */
		if (ucActionCode != TDLS_FRM_ACTION_CONFIRM) {
			/* 3. Frame Formation - (4) Capability: 0x31 0x04, privacy bit will be set */
			u2CapInfo = assocBuildCapabilityInfo(prAdapter, prStaRec);
			WLAN_SET_FIELD_16(pPkt, u2CapInfo);
			LR_TDLS_FME_FIELD_FILL(2);

			/* 4. Append general IEs */
			/*
			   TODO check HT: prAdapter->rWifiVar.rConnSettings.uc2G4BandwidthMode
			   must be CONFIG_BW_20_40M.

			   TODO check HT: HT_CAP_INFO_40M_INTOLERANT must be clear if
			   Tdls 20/40 is enabled.
			 */
			u4IeLen = TdlsFrameGeneralIeAppend(prAdapter, prStaRec, pPkt);
			LR_TDLS_FME_FIELD_FILL(u4IeLen);

			/* 5. Frame Formation - Extended capabilities element */
			EXT_CAP_IE(pPkt)->ucId = ELEM_ID_EXTENDED_CAP;
			EXT_CAP_IE(pPkt)->ucLength = 5;

			EXT_CAP_IE(pPkt)->aucCapabilities[0] = 0x00;	/* bit0 ~ bit7 */
			EXT_CAP_IE(pPkt)->aucCapabilities[1] = 0x00;	/* bit8 ~ bit15 */
			EXT_CAP_IE(pPkt)->aucCapabilities[2] = 0x00;	/* bit16 ~ bit23 */
			EXT_CAP_IE(pPkt)->aucCapabilities[3] = 0x00;	/* bit24 ~ bit31 */
			EXT_CAP_IE(pPkt)->aucCapabilities[4] = 0x00;	/* bit32 ~ bit39 */

			/* if (prCmd->ucExCap & TDLS_EX_CAP_PEER_UAPSD) */
			EXT_CAP_IE(pPkt)->aucCapabilities[3] |= BIT((28 - 24));
			/* if (prCmd->ucExCap & TDLS_EX_CAP_CHAN_SWITCH) */
			EXT_CAP_IE(pPkt)->aucCapabilities[3] |= BIT((30 - 24));
			/* if (prCmd->ucExCap & TDLS_EX_CAP_TDLS) */
			EXT_CAP_IE(pPkt)->aucCapabilities[4] |= BIT((37 - 32));

			u4IeLen = IE_SIZE(pPkt);
			LR_TDLS_FME_FIELD_FILL(u4IeLen);
		} else {
			/* 5. Frame Formation - WMM Information element */
			if (IS_FEATURE_ENABLED(prAdapter->rWifiVar.ucQoS)) {

				/* Add WMM IE */ /* try to reuse p2p path */
				u4IeLen =
				    mqmGenerateWmmInfoIEByStaRec(prAdapter, prBssInfo, prStaRec,
								 pPkt);
				pPkt += u4IeLen;
				u4PktLen += u4IeLen;
			}

		}
	}

	/* 6. Frame Formation - 20/40 BSS Coexistence */
	/*
	   Follow WiFi test plan, add 20/40 element to request/response/confirm.
	 */
#if 0
	if (prGlueInfo->fgTdlsIs2040Supported == TRUE) {
		/*
		   bit0 = 1: The Information Request field is used to indicate that a
		   transmitting STA is requesting the recipient to transmit a 20/40 BSS
		   Coexistence Management frame with the transmitting STA as the
		   recipient.

		   bit1 = 0: The Forty MHz Intolerant field is set to 1 to prohibit an AP
		   that receives this information or reports of this information from
		   operating a 20/40 MHz BSS.

		   bit2 = 0: The 20 MHz BSS Width Request field is set to 1 to prohibit
		   a receiving AP from operating its BSS as a 20/40 MHz BSS.
		 */
		BSS_20_40_COEXIST_IE(pPkt)->ucId = ELEM_ID_20_40_BSS_COEXISTENCE;
		BSS_20_40_COEXIST_IE(pPkt)->ucLength = 1;
		BSS_20_40_COEXIST_IE(pPkt)->ucData = 0x01;
		LR_TDLS_FME_FIELD_FILL(3);
	}
#endif
	/* 6. Frame Formation - HT Operation element */
	/* u4IeLen = rlmFillHtOpIeBody(prBssInfo, pPkt); */
	/* LR_TDLS_FME_FIELD_FILL(u4IeLen); */

	/* 7. Frame Formation - Link identifier element */
	/* Note: Link ID sequence must be correct; Or the calculated MIC will be error */
	TDLS_LINK_IDENTIFIER_IE(pPkt)->ucId = ELEM_ID_LINK_IDENTIFIER;
	TDLS_LINK_IDENTIFIER_IE(pPkt)->ucLength = 18;

	kalMemCopy(TDLS_LINK_IDENTIFIER_IE(pPkt)->aBSSID, prBssInfo->aucBSSID, 6);

	switch (ucActionCode) {
	case TDLS_FRM_ACTION_SETUP_REQ:
	case TDLS_FRM_ACTION_CONFIRM:
	case TDLS_FRM_ACTION_DISCOVERY_RSP:
	default:
		/* we are initiator */
		pucInitiator = prBssInfo->aucOwnMacAddr;
		pucResponder = pPeerMac;

		if (prStaRec != NULL)
			prStaRec->flgTdlsIsInitiator = TRUE;
		break;

	case TDLS_FRM_ACTION_SETUP_RSP:
		/* peer is initiator */
		pucInitiator = pPeerMac;
		pucResponder = prBssInfo->aucOwnMacAddr;

		if (prStaRec != NULL)
			prStaRec->flgTdlsIsInitiator = FALSE;
		break;

	case TDLS_FRM_ACTION_TEARDOWN:
		if (prStaRec != NULL) {
			if (prStaRec->flgTdlsIsInitiator == TRUE) {
				/* we are initiator */
				pucInitiator = prBssInfo->aucOwnMacAddr;
				pucResponder = pPeerMac;
			} else {
				/* peer is initiator */
				pucInitiator = pPeerMac;
				pucResponder = prBssInfo->aucOwnMacAddr;
			}
		} else {
			/* peer is initiator */
			pucInitiator = pPeerMac;
			pucResponder = prBssInfo->aucOwnMacAddr;
		}
		break;
	}

	kalMemCopy(TDLS_LINK_IDENTIFIER_IE(pPkt)->aInitiator, pucInitiator, 6);
	kalMemCopy(TDLS_LINK_IDENTIFIER_IE(pPkt)->aResponder, pucResponder, 6);

	u4IeLen = IE_SIZE(pPkt);
	LR_TDLS_FME_FIELD_FILL(u4IeLen);

	/* 8. Append security IEs */
	if ((ucActionCode != TDLS_FRM_ACTION_TEARDOWN) && (pAppendIe != NULL)) {
		kalMemCopy(pPkt, pAppendIe, AppendIeLen);
		LR_TDLS_FME_FIELD_FILL(AppendIeLen);
	}

	/* 10. send the data or management frame */
	if (ucActionCode != TDLS_FRM_ACTION_DISCOVERY_RSP) {
		/* 9. Update packet length */
		prMsduInfo->len = u4PktLen;

		wlanHardStartXmit(prMsduInfo, prMsduInfo->dev);
	} else {
		prMsduInfoMgmt->ucPacketType = TX_PACKET_TYPE_MGMT;
		prMsduInfoMgmt->ucStaRecIndex = prBssInfo->prStaRecOfAP->ucIndex;
		prMsduInfoMgmt->ucBssIndex = prBssInfo->ucBssIndex;
		prMsduInfoMgmt->ucMacHeaderLength = WLAN_MAC_MGMT_HEADER_LEN;
		prMsduInfoMgmt->fgIs802_1x = FALSE;
		prMsduInfoMgmt->fgIs802_11 = TRUE;
		prMsduInfoMgmt->u2FrameLength = u4PktLen;
		prMsduInfoMgmt->ucTxSeqNum = nicIncreaseTxSeqNum(prAdapter);
		prMsduInfoMgmt->pfTxDoneHandler = NULL;


		/* Send them to HW queue */
		nicTxEnqueueMsdu(prAdapter, prMsduInfoMgmt);
	}

	return TDLS_STATUS_SUCCESS;
}


#endif				/* CFG_SUPPORT_TDLS */

/* End of tdls.c */
