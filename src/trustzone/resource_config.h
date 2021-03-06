#ifndef _RESOURCE_CONFIG_H_
#define _RESOURCE_CONFIG_H_

#if defined(__cplusplus)
extern "C" {
#endif /* __cplusplus */


/***********************************************************************************************************************
 * Initialize TrustZone
 **********************************************************************************************************************/
void BOARD_InitTrustZone(void);

/***********************************************************************************************************************
 * Initialize AHBSE
 **********************************************************************************************************************/
void BOARD_InitAHBSE(void);

/***********************************************************************************************************************
 * Initialize TEE
 **********************************************************************************************************************/
void BOARD_InitTEE(void);

#if defined(__cplusplus)
}
#endif

#endif /* _RESOURCE_CONFIG_H_ */
