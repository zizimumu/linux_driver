
#include <linux/module.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/of_platform.h>
#include <linux/err.h>
#include <linux/io.h>
#include <linux/clk.h>
#include <linux/interrupt.h>
#include <linux/hrtimer.h>
#include <linux/jiffies.h>


#define TC_CCR_CLKEN (0x1u << 0) /**< \brief (TC_CCR) Counter Clock Enable Command */
#define TC_CCR_CLKDIS (0x1u << 1) /**< \brief (TC_CCR) Counter Clock Disable Command */
#define TC_CCR_SWTRG (0x1u << 2) /**< \brief (TC_CCR) Software Trigger Command */
/* -------- TC_CMR : (TC Offset: N/A) Channel Mode Register -------- */
#define TC_CMR_TCCLKS_Pos 0
#define TC_CMR_TCCLKS_Msk (0x7u << TC_CMR_TCCLKS_Pos) /**< \brief (TC_CMR) Clock Selection */
#define TC_CMR_TCCLKS(value) ((TC_CMR_TCCLKS_Msk & ((value) << TC_CMR_TCCLKS_Pos)))
#define   TC_CMR_TCCLKS_TIMER_CLOCK1 (0x0u << 0) /**< \brief (TC_CMR) Clock selected: internal TIMER_CLOCK1 clock signal (from PMC) */
#define   TC_CMR_TCCLKS_TIMER_CLOCK2 (0x1u << 0) /**< \brief (TC_CMR) Clock selected: internal TIMER_CLOCK2 clock signal (from PMC) */
#define   TC_CMR_TCCLKS_TIMER_CLOCK3 (0x2u << 0) /**< \brief (TC_CMR) Clock selected: internal TIMER_CLOCK3 clock signal (from PMC) */
#define   TC_CMR_TCCLKS_TIMER_CLOCK4 (0x3u << 0) /**< \brief (TC_CMR) Clock selected: internal TIMER_CLOCK4 clock signal (from PMC) */
#define   TC_CMR_TCCLKS_TIMER_CLOCK5 (0x4u << 0) /**< \brief (TC_CMR) Clock selected: internal TIMER_CLOCK5 clock signal (from PMC) */
#define   TC_CMR_TCCLKS_XC0 (0x5u << 0) /**< \brief (TC_CMR) Clock selected: XC0 */
#define   TC_CMR_TCCLKS_XC1 (0x6u << 0) /**< \brief (TC_CMR) Clock selected: XC1 */
#define   TC_CMR_TCCLKS_XC2 (0x7u << 0) /**< \brief (TC_CMR) Clock selected: XC2 */
#define TC_CMR_CLKI (0x1u << 3) /**< \brief (TC_CMR) Clock Invert */
#define TC_CMR_BURST_Pos 4
#define TC_CMR_BURST_Msk (0x3u << TC_CMR_BURST_Pos) /**< \brief (TC_CMR) Burst Signal Selection */
#define TC_CMR_BURST(value) ((TC_CMR_BURST_Msk & ((value) << TC_CMR_BURST_Pos)))
#define   TC_CMR_BURST_NONE (0x0u << 4) /**< \brief (TC_CMR) The clock is not gated by an external signal. */
#define   TC_CMR_BURST_XC0 (0x1u << 4) /**< \brief (TC_CMR) XC0 is ANDed with the selected clock. */
#define   TC_CMR_BURST_XC1 (0x2u << 4) /**< \brief (TC_CMR) XC1 is ANDed with the selected clock. */
#define   TC_CMR_BURST_XC2 (0x3u << 4) /**< \brief (TC_CMR) XC2 is ANDed with the selected clock. */
#define TC_CMR_LDBSTOP (0x1u << 6) /**< \brief (TC_CMR) Counter Clock Stopped with RB Loading */
#define TC_CMR_LDBDIS (0x1u << 7) /**< \brief (TC_CMR) Counter Clock Disable with RB Loading */
#define TC_CMR_ETRGEDG_Pos 8
#define TC_CMR_ETRGEDG_Msk (0x3u << TC_CMR_ETRGEDG_Pos) /**< \brief (TC_CMR) External Trigger Edge Selection */
#define TC_CMR_ETRGEDG(value) ((TC_CMR_ETRGEDG_Msk & ((value) << TC_CMR_ETRGEDG_Pos)))
#define   TC_CMR_ETRGEDG_NONE (0x0u << 8) /**< \brief (TC_CMR) The clock is not gated by an external signal. */
#define   TC_CMR_ETRGEDG_RISING (0x1u << 8) /**< \brief (TC_CMR) Rising edge */
#define   TC_CMR_ETRGEDG_FALLING (0x2u << 8) /**< \brief (TC_CMR) Falling edge */
#define   TC_CMR_ETRGEDG_EDGE (0x3u << 8) /**< \brief (TC_CMR) Each edge */
#define TC_CMR_ABETRG (0x1u << 10) /**< \brief (TC_CMR) TIOAx or TIOBx External Trigger Selection */
#define TC_CMR_CPCTRG (0x1u << 14) /**< \brief (TC_CMR) RC Compare Trigger Enable */
#define TC_CMR_WAVE (0x1u << 15) /**< \brief (TC_CMR) Waveform Mode */
#define TC_CMR_LDRA_Pos 16
#define TC_CMR_LDRA_Msk (0x3u << TC_CMR_LDRA_Pos) /**< \brief (TC_CMR) RA Loading Edge Selection */
#define TC_CMR_LDRA(value) ((TC_CMR_LDRA_Msk & ((value) << TC_CMR_LDRA_Pos)))
#define   TC_CMR_LDRA_NONE (0x0u << 16) /**< \brief (TC_CMR) None */
#define   TC_CMR_LDRA_RISING (0x1u << 16) /**< \brief (TC_CMR) Rising edge of TIOAx */
#define   TC_CMR_LDRA_FALLING (0x2u << 16) /**< \brief (TC_CMR) Falling edge of TIOAx */
#define   TC_CMR_LDRA_EDGE (0x3u << 16) /**< \brief (TC_CMR) Each edge of TIOAx */
#define TC_CMR_LDRB_Pos 18
#define TC_CMR_LDRB_Msk (0x3u << TC_CMR_LDRB_Pos) /**< \brief (TC_CMR) RB Loading Edge Selection */
#define TC_CMR_LDRB(value) ((TC_CMR_LDRB_Msk & ((value) << TC_CMR_LDRB_Pos)))
#define   TC_CMR_LDRB_NONE (0x0u << 18) /**< \brief (TC_CMR) None */
#define   TC_CMR_LDRB_RISING (0x1u << 18) /**< \brief (TC_CMR) Rising edge of TIOAx */
#define   TC_CMR_LDRB_FALLING (0x2u << 18) /**< \brief (TC_CMR) Falling edge of TIOAx */
#define   TC_CMR_LDRB_EDGE (0x3u << 18) /**< \brief (TC_CMR) Each edge of TIOAx */
#define TC_CMR_SBSMPLR_Pos 20
#define TC_CMR_SBSMPLR_Msk (0x7u << TC_CMR_SBSMPLR_Pos) /**< \brief (TC_CMR) Loading Edge Subsampling Ratio */
#define TC_CMR_SBSMPLR(value) ((TC_CMR_SBSMPLR_Msk & ((value) << TC_CMR_SBSMPLR_Pos)))
#define   TC_CMR_SBSMPLR_ONE (0x0u << 20) /**< \brief (TC_CMR) Load a Capture register each selected edge */
#define   TC_CMR_SBSMPLR_HALF (0x1u << 20) /**< \brief (TC_CMR) Load a Capture register every 2 selected edges */
#define   TC_CMR_SBSMPLR_FOURTH (0x2u << 20) /**< \brief (TC_CMR) Load a Capture register every 4 selected edges */
#define   TC_CMR_SBSMPLR_EIGHTH (0x3u << 20) /**< \brief (TC_CMR) Load a Capture register every 8 selected edges */
#define   TC_CMR_SBSMPLR_SIXTEENTH (0x4u << 20) /**< \brief (TC_CMR) Load a Capture register every 16 selected edges */
#define TC_CMR_CPCSTOP (0x1u << 6) /**< \brief (TC_CMR) Counter Clock Stopped with RC Compare */
#define TC_CMR_CPCDIS (0x1u << 7) /**< \brief (TC_CMR) Counter Clock Disable with RC Compare */
#define TC_CMR_EEVTEDG_Pos 8
#define TC_CMR_EEVTEDG_Msk (0x3u << TC_CMR_EEVTEDG_Pos) /**< \brief (TC_CMR) External Event Edge Selection */
#define TC_CMR_EEVTEDG(value) ((TC_CMR_EEVTEDG_Msk & ((value) << TC_CMR_EEVTEDG_Pos)))
#define   TC_CMR_EEVTEDG_NONE (0x0u << 8) /**< \brief (TC_CMR) None */
#define   TC_CMR_EEVTEDG_RISING (0x1u << 8) /**< \brief (TC_CMR) Rising edge */
#define   TC_CMR_EEVTEDG_FALLING (0x2u << 8) /**< \brief (TC_CMR) Falling edge */
#define   TC_CMR_EEVTEDG_EDGE (0x3u << 8) /**< \brief (TC_CMR) Each edge */
#define TC_CMR_EEVT_Pos 10
#define TC_CMR_EEVT_Msk (0x3u << TC_CMR_EEVT_Pos) /**< \brief (TC_CMR) External Event Selection */
#define TC_CMR_EEVT(value) ((TC_CMR_EEVT_Msk & ((value) << TC_CMR_EEVT_Pos)))
#define   TC_CMR_EEVT_TIOB (0x0u << 10) /**< \brief (TC_CMR) TIOB */
#define   TC_CMR_EEVT_XC0 (0x1u << 10) /**< \brief (TC_CMR) XC0 */
#define   TC_CMR_EEVT_XC1 (0x2u << 10) /**< \brief (TC_CMR) XC1 */
#define   TC_CMR_EEVT_XC2 (0x3u << 10) /**< \brief (TC_CMR) XC2 */
#define TC_CMR_ENETRG (0x1u << 12) /**< \brief (TC_CMR) External Event Trigger Enable */
#define TC_CMR_WAVSEL_Pos 13
#define TC_CMR_WAVSEL_Msk (0x3u << TC_CMR_WAVSEL_Pos) /**< \brief (TC_CMR) Waveform Selection */
#define TC_CMR_WAVSEL(value) ((TC_CMR_WAVSEL_Msk & ((value) << TC_CMR_WAVSEL_Pos)))
#define   TC_CMR_WAVSEL_UP (0x0u << 13) /**< \brief (TC_CMR) UP mode without automatic trigger on RC Compare */
#define   TC_CMR_WAVSEL_UPDOWN (0x1u << 13) /**< \brief (TC_CMR) UPDOWN mode without automatic trigger on RC Compare */
#define   TC_CMR_WAVSEL_UP_RC (0x2u << 13) /**< \brief (TC_CMR) UP mode with automatic trigger on RC Compare */
#define   TC_CMR_WAVSEL_UPDOWN_RC (0x3u << 13) /**< \brief (TC_CMR) UPDOWN mode with automatic trigger on RC Compare */
#define TC_CMR_ACPA_Pos 16
#define TC_CMR_ACPA_Msk (0x3u << TC_CMR_ACPA_Pos) /**< \brief (TC_CMR) RA Compare Effect on TIOAx */
#define TC_CMR_ACPA(value) ((TC_CMR_ACPA_Msk & ((value) << TC_CMR_ACPA_Pos)))
#define   TC_CMR_ACPA_NONE (0x0u << 16) /**< \brief (TC_CMR) None */
#define   TC_CMR_ACPA_SET (0x1u << 16) /**< \brief (TC_CMR) Set */
#define   TC_CMR_ACPA_CLEAR (0x2u << 16) /**< \brief (TC_CMR) Clear */
#define   TC_CMR_ACPA_TOGGLE (0x3u << 16) /**< \brief (TC_CMR) Toggle */
#define TC_CMR_ACPC_Pos 18
#define TC_CMR_ACPC_Msk (0x3u << TC_CMR_ACPC_Pos) /**< \brief (TC_CMR) RC Compare Effect on TIOAx */
#define TC_CMR_ACPC(value) ((TC_CMR_ACPC_Msk & ((value) << TC_CMR_ACPC_Pos)))
#define   TC_CMR_ACPC_NONE (0x0u << 18) /**< \brief (TC_CMR) None */
#define   TC_CMR_ACPC_SET (0x1u << 18) /**< \brief (TC_CMR) Set */
#define   TC_CMR_ACPC_CLEAR (0x2u << 18) /**< \brief (TC_CMR) Clear */
#define   TC_CMR_ACPC_TOGGLE (0x3u << 18) /**< \brief (TC_CMR) Toggle */
#define TC_CMR_AEEVT_Pos 20
#define TC_CMR_AEEVT_Msk (0x3u << TC_CMR_AEEVT_Pos) /**< \brief (TC_CMR) External Event Effect on TIOAx */
#define TC_CMR_AEEVT(value) ((TC_CMR_AEEVT_Msk & ((value) << TC_CMR_AEEVT_Pos)))
#define   TC_CMR_AEEVT_NONE (0x0u << 20) /**< \brief (TC_CMR) None */
#define   TC_CMR_AEEVT_SET (0x1u << 20) /**< \brief (TC_CMR) Set */
#define   TC_CMR_AEEVT_CLEAR (0x2u << 20) /**< \brief (TC_CMR) Clear */
#define   TC_CMR_AEEVT_TOGGLE (0x3u << 20) /**< \brief (TC_CMR) Toggle */
#define TC_CMR_ASWTRG_Pos 22
#define TC_CMR_ASWTRG_Msk (0x3u << TC_CMR_ASWTRG_Pos) /**< \brief (TC_CMR) Software Trigger Effect on TIOAx */
#define TC_CMR_ASWTRG(value) ((TC_CMR_ASWTRG_Msk & ((value) << TC_CMR_ASWTRG_Pos)))
#define   TC_CMR_ASWTRG_NONE (0x0u << 22) /**< \brief (TC_CMR) None */
#define   TC_CMR_ASWTRG_SET (0x1u << 22) /**< \brief (TC_CMR) Set */
#define   TC_CMR_ASWTRG_CLEAR (0x2u << 22) /**< \brief (TC_CMR) Clear */
#define   TC_CMR_ASWTRG_TOGGLE (0x3u << 22) /**< \brief (TC_CMR) Toggle */
#define TC_CMR_BCPB_Pos 24
#define TC_CMR_BCPB_Msk (0x3u << TC_CMR_BCPB_Pos) /**< \brief (TC_CMR) RB Compare Effect on TIOBx */
#define TC_CMR_BCPB(value) ((TC_CMR_BCPB_Msk & ((value) << TC_CMR_BCPB_Pos)))
#define   TC_CMR_BCPB_NONE (0x0u << 24) /**< \brief (TC_CMR) None */
#define   TC_CMR_BCPB_SET (0x1u << 24) /**< \brief (TC_CMR) Set */
#define   TC_CMR_BCPB_CLEAR (0x2u << 24) /**< \brief (TC_CMR) Clear */
#define   TC_CMR_BCPB_TOGGLE (0x3u << 24) /**< \brief (TC_CMR) Toggle */
#define TC_CMR_BCPC_Pos 26
#define TC_CMR_BCPC_Msk (0x3u << TC_CMR_BCPC_Pos) /**< \brief (TC_CMR) RC Compare Effect on TIOBx */
#define TC_CMR_BCPC(value) ((TC_CMR_BCPC_Msk & ((value) << TC_CMR_BCPC_Pos)))
#define   TC_CMR_BCPC_NONE (0x0u << 26) /**< \brief (TC_CMR) None */
#define   TC_CMR_BCPC_SET (0x1u << 26) /**< \brief (TC_CMR) Set */
#define   TC_CMR_BCPC_CLEAR (0x2u << 26) /**< \brief (TC_CMR) Clear */
#define   TC_CMR_BCPC_TOGGLE (0x3u << 26) /**< \brief (TC_CMR) Toggle */
#define TC_CMR_BEEVT_Pos 28
#define TC_CMR_BEEVT_Msk (0x3u << TC_CMR_BEEVT_Pos) /**< \brief (TC_CMR) External Event Effect on TIOBx */
#define TC_CMR_BEEVT(value) ((TC_CMR_BEEVT_Msk & ((value) << TC_CMR_BEEVT_Pos)))
#define   TC_CMR_BEEVT_NONE (0x0u << 28) /**< \brief (TC_CMR) None */
#define   TC_CMR_BEEVT_SET (0x1u << 28) /**< \brief (TC_CMR) Set */
#define   TC_CMR_BEEVT_CLEAR (0x2u << 28) /**< \brief (TC_CMR) Clear */
#define   TC_CMR_BEEVT_TOGGLE (0x3u << 28) /**< \brief (TC_CMR) Toggle */
#define TC_CMR_BSWTRG_Pos 30
#define TC_CMR_BSWTRG_Msk (0x3u << TC_CMR_BSWTRG_Pos) /**< \brief (TC_CMR) Software Trigger Effect on TIOBx */
#define TC_CMR_BSWTRG(value) ((TC_CMR_BSWTRG_Msk & ((value) << TC_CMR_BSWTRG_Pos)))
#define   TC_CMR_BSWTRG_NONE (0x0u << 30) /**< \brief (TC_CMR) None */
#define   TC_CMR_BSWTRG_SET (0x1u << 30) /**< \brief (TC_CMR) Set */
#define   TC_CMR_BSWTRG_CLEAR (0x2u << 30) /**< \brief (TC_CMR) Clear */
#define   TC_CMR_BSWTRG_TOGGLE (0x3u << 30) /**< \brief (TC_CMR) Toggle */
/* -------- TC_SMMR : (TC Offset: N/A) Stepper Motor Mode Register -------- */
#define TC_SMMR_GCEN (0x1u << 0) /**< \brief (TC_SMMR) Gray Count Enable */
#define TC_SMMR_DOWN (0x1u << 1) /**< \brief (TC_SMMR) Down Count */
/* -------- TC_RAB : (TC Offset: N/A) Register AB -------- */
#define TC_RAB_RAB_Pos 0
#define TC_RAB_RAB_Msk (0xffffffffu << TC_RAB_RAB_Pos) /**< \brief (TC_RAB) Register A or Register B */
/* -------- TC_CV : (TC Offset: N/A) Counter Value -------- */
#define TC_CV_CV_Pos 0
#define TC_CV_CV_Msk (0xffffffffu << TC_CV_CV_Pos) /**< \brief (TC_CV) Counter Value */
/* -------- TC_RA : (TC Offset: N/A) Register A -------- */
#define TC_RA_RA_Pos 0
#define TC_RA_RA_Msk (0xffffffffu << TC_RA_RA_Pos) /**< \brief (TC_RA) Register A */
#define TC_RA_RA(value) ((TC_RA_RA_Msk & ((value) << TC_RA_RA_Pos)))
/* -------- TC_RB : (TC Offset: N/A) Register B -------- */
#define TC_RB_RB_Pos 0
#define TC_RB_RB_Msk (0xffffffffu << TC_RB_RB_Pos) /**< \brief (TC_RB) Register B */
#define TC_RB_RB(value) ((TC_RB_RB_Msk & ((value) << TC_RB_RB_Pos)))
/* -------- TC_RC : (TC Offset: N/A) Register C -------- */
#define TC_RC_RC_Pos 0
#define TC_RC_RC_Msk (0xffffffffu << TC_RC_RC_Pos) /**< \brief (TC_RC) Register C */
#define TC_RC_RC(value) ((TC_RC_RC_Msk & ((value) << TC_RC_RC_Pos)))
/* -------- TC_SR : (TC Offset: N/A) Status Register -------- */
#define TC_SR_COVFS (0x1u << 0) /**< \brief (TC_SR) Counter Overflow Status */
#define TC_SR_LOVRS (0x1u << 1) /**< \brief (TC_SR) Load Overrun Status */
#define TC_SR_CPAS (0x1u << 2) /**< \brief (TC_SR) RA Compare Status */
#define TC_SR_CPBS (0x1u << 3) /**< \brief (TC_SR) RB Compare Status */
#define TC_SR_CPCS (0x1u << 4) /**< \brief (TC_SR) RC Compare Status */
#define TC_SR_LDRAS (0x1u << 5) /**< \brief (TC_SR) RA Loading Status */
#define TC_SR_LDRBS (0x1u << 6) /**< \brief (TC_SR) RB Loading Status */
#define TC_SR_ETRGS (0x1u << 7) /**< \brief (TC_SR) External Trigger Status */
#define TC_SR_CLKSTA (0x1u << 16) /**< \brief (TC_SR) Clock Enabling Status */
#define TC_SR_MTIOA (0x1u << 17) /**< \brief (TC_SR) TIOAx Mirror */
#define TC_SR_MTIOB (0x1u << 18) /**< \brief (TC_SR) TIOBx Mirror */
/* -------- TC_IER : (TC Offset: N/A) Interrupt Enable Register -------- */
#define TC_IER_COVFS (0x1u << 0) /**< \brief (TC_IER) Counter Overflow */
#define TC_IER_LOVRS (0x1u << 1) /**< \brief (TC_IER) Load Overrun */
#define TC_IER_CPAS (0x1u << 2) /**< \brief (TC_IER) RA Compare */
#define TC_IER_CPBS (0x1u << 3) /**< \brief (TC_IER) RB Compare */
#define TC_IER_CPCS (0x1u << 4) /**< \brief (TC_IER) RC Compare */
#define TC_IER_LDRAS (0x1u << 5) /**< \brief (TC_IER) RA Loading */
#define TC_IER_LDRBS (0x1u << 6) /**< \brief (TC_IER) RB Loading */
#define TC_IER_ETRGS (0x1u << 7) /**< \brief (TC_IER) External Trigger */
/* -------- TC_IDR : (TC Offset: N/A) Interrupt Disable Register -------- */
#define TC_IDR_COVFS (0x1u << 0) /**< \brief (TC_IDR) Counter Overflow */
#define TC_IDR_LOVRS (0x1u << 1) /**< \brief (TC_IDR) Load Overrun */
#define TC_IDR_CPAS (0x1u << 2) /**< \brief (TC_IDR) RA Compare */
#define TC_IDR_CPBS (0x1u << 3) /**< \brief (TC_IDR) RB Compare */
#define TC_IDR_CPCS (0x1u << 4) /**< \brief (TC_IDR) RC Compare */
#define TC_IDR_LDRAS (0x1u << 5) /**< \brief (TC_IDR) RA Loading */
#define TC_IDR_LDRBS (0x1u << 6) /**< \brief (TC_IDR) RB Loading */
#define TC_IDR_ETRGS (0x1u << 7) /**< \brief (TC_IDR) External Trigger */
/* -------- TC_IMR : (TC Offset: N/A) Interrupt Mask Register -------- */
#define TC_IMR_COVFS (0x1u << 0) /**< \brief (TC_IMR) Counter Overflow */
#define TC_IMR_LOVRS (0x1u << 1) /**< \brief (TC_IMR) Load Overrun */
#define TC_IMR_CPAS (0x1u << 2) /**< \brief (TC_IMR) RA Compare */
#define TC_IMR_CPBS (0x1u << 3) /**< \brief (TC_IMR) RB Compare */
#define TC_IMR_CPCS (0x1u << 4) /**< \brief (TC_IMR) RC Compare */
#define TC_IMR_LDRAS (0x1u << 5) /**< \brief (TC_IMR) RA Loading */
#define TC_IMR_LDRBS (0x1u << 6) /**< \brief (TC_IMR) RB Loading */
#define TC_IMR_ETRGS (0x1u << 7) /**< \brief (TC_IMR) External Trigger */
/* -------- TC_EMR : (TC Offset: N/A) Extended Mode Register -------- */
#define TC_EMR_TRIGSRCA_Pos 0
#define TC_EMR_TRIGSRCA_Msk (0x3u << TC_EMR_TRIGSRCA_Pos) /**< \brief (TC_EMR) Trigger Source for Input A */
#define TC_EMR_TRIGSRCA(value) ((TC_EMR_TRIGSRCA_Msk & ((value) << TC_EMR_TRIGSRCA_Pos)))
#define   TC_EMR_TRIGSRCA_EXTERNAL_TIOAx (0x0u << 0) /**< \brief (TC_EMR) The trigger/capture input A is driven by external pin TIOAx */
#define   TC_EMR_TRIGSRCA_PWMx (0x1u << 0) /**< \brief (TC_EMR) The trigger/capture input A is driven internally by PWMx */
#define TC_EMR_TRIGSRCB_Pos 4
#define TC_EMR_TRIGSRCB_Msk (0x3u << TC_EMR_TRIGSRCB_Pos) /**< \brief (TC_EMR) Trigger Source for Input B */
#define TC_EMR_TRIGSRCB(value) ((TC_EMR_TRIGSRCB_Msk & ((value) << TC_EMR_TRIGSRCB_Pos)))
#define   TC_EMR_TRIGSRCB_EXTERNAL_TIOBx (0x0u << 4) /**< \brief (TC_EMR) The trigger/capture input B is driven by external pin TIOBx */
#define   TC_EMR_TRIGSRCB_PWMx (0x1u << 4) /**< \brief (TC_EMR) The trigger/capture input B is driven internally by PWMx */
#define TC_EMR_NODIVCLK (0x1u << 8) /**< \brief (TC_EMR) No Divided Clock */
/* -------- TC_BCR : (TC Offset: 0xC0) Block Control Register -------- */
#define TC_BCR_SYNC (0x1u << 0) /**< \brief (TC_BCR) Synchro Command */
/* -------- TC_BMR : (TC Offset: 0xC4) Block Mode Register -------- */
#define TC_BMR_TC0XC0S_Pos 0
#define TC_BMR_TC0XC0S_Msk (0x3u << TC_BMR_TC0XC0S_Pos) /**< \brief (TC_BMR) External Clock Signal 0 Selection */
#define TC_BMR_TC0XC0S(value) ((TC_BMR_TC0XC0S_Msk & ((value) << TC_BMR_TC0XC0S_Pos)))
#define   TC_BMR_TC0XC0S_TCLK0 (0x0u << 0) /**< \brief (TC_BMR) Signal connected to XC0: TCLK0 */
#define   TC_BMR_TC0XC0S_TIOA1 (0x2u << 0) /**< \brief (TC_BMR) Signal connected to XC0: TIOA1 */
#define   TC_BMR_TC0XC0S_TIOA2 (0x3u << 0) /**< \brief (TC_BMR) Signal connected to XC0: TIOA2 */
#define TC_BMR_TC1XC1S_Pos 2
#define TC_BMR_TC1XC1S_Msk (0x3u << TC_BMR_TC1XC1S_Pos) /**< \brief (TC_BMR) External Clock Signal 1 Selection */
#define TC_BMR_TC1XC1S(value) ((TC_BMR_TC1XC1S_Msk & ((value) << TC_BMR_TC1XC1S_Pos)))
#define   TC_BMR_TC1XC1S_TCLK1 (0x0u << 2) /**< \brief (TC_BMR) Signal connected to XC1: TCLK1 */
#define   TC_BMR_TC1XC1S_TIOA0 (0x2u << 2) /**< \brief (TC_BMR) Signal connected to XC1: TIOA0 */
#define   TC_BMR_TC1XC1S_TIOA2 (0x3u << 2) /**< \brief (TC_BMR) Signal connected to XC1: TIOA2 */
#define TC_BMR_TC2XC2S_Pos 4
#define TC_BMR_TC2XC2S_Msk (0x3u << TC_BMR_TC2XC2S_Pos) /**< \brief (TC_BMR) External Clock Signal 2 Selection */
#define TC_BMR_TC2XC2S(value) ((TC_BMR_TC2XC2S_Msk & ((value) << TC_BMR_TC2XC2S_Pos)))
#define   TC_BMR_TC2XC2S_TCLK2 (0x0u << 4) /**< \brief (TC_BMR) Signal connected to XC2: TCLK2 */
#define   TC_BMR_TC2XC2S_TIOA0 (0x2u << 4) /**< \brief (TC_BMR) Signal connected to XC2: TIOA0 */
#define   TC_BMR_TC2XC2S_TIOA1 (0x3u << 4) /**< \brief (TC_BMR) Signal connected to XC2: TIOA1 */
#define TC_BMR_QDEN (0x1u << 8) /**< \brief (TC_BMR) Quadrature Decoder Enabled */
#define TC_BMR_POSEN (0x1u << 9) /**< \brief (TC_BMR) Position Enabled */
#define TC_BMR_SPEEDEN (0x1u << 10) /**< \brief (TC_BMR) Speed Enabled */
#define TC_BMR_QDTRANS (0x1u << 11) /**< \brief (TC_BMR) Quadrature Decoding Transparent */
#define TC_BMR_EDGPHA (0x1u << 12) /**< \brief (TC_BMR) Edge on PHA Count Mode */
#define TC_BMR_INVA (0x1u << 13) /**< \brief (TC_BMR) Inverted PHA */
#define TC_BMR_INVB (0x1u << 14) /**< \brief (TC_BMR) Inverted PHB */
#define TC_BMR_INVIDX (0x1u << 15) /**< \brief (TC_BMR) Inverted Index */
#define TC_BMR_SWAP (0x1u << 16) /**< \brief (TC_BMR) Swap PHA and PHB */
#define TC_BMR_IDXPHB (0x1u << 17) /**< \brief (TC_BMR) Index Pin is PHB Pin */
#define TC_BMR_AUTOC (0x1u << 18) /**< \brief (TC_BMR) Auto-Correction of missing pulses */
#define   TC_BMR_AUTOC_DISABLED (0x0u << 18) /**< \brief (TC_BMR) The detection and auto-correction function is disabled. */
#define   TC_BMR_AUTOC_ENABLED (0x1u << 18) /**< \brief (TC_BMR) The detection and auto-correction function is enabled. */
#define TC_BMR_MAXFILT_Pos 20
#define TC_BMR_MAXFILT_Msk (0x3fu << TC_BMR_MAXFILT_Pos) /**< \brief (TC_BMR) Maximum Filter */
#define TC_BMR_MAXFILT(value) ((TC_BMR_MAXFILT_Msk & ((value) << TC_BMR_MAXFILT_Pos)))
#define TC_BMR_MAXCMP_Pos 26
#define TC_BMR_MAXCMP_Msk (0xfu << TC_BMR_MAXCMP_Pos) /**< \brief (TC_BMR) Maximum Consecutive Missing Pulses */
#define TC_BMR_MAXCMP(value) ((TC_BMR_MAXCMP_Msk & ((value) << TC_BMR_MAXCMP_Pos)))
/* -------- TC_QIER : (TC Offset: 0xC8) QDEC Interrupt Enable Register -------- */
#define TC_QIER_IDX (0x1u << 0) /**< \brief (TC_QIER) Index */
#define TC_QIER_DIRCHG (0x1u << 1) /**< \brief (TC_QIER) Direction Change */
#define TC_QIER_QERR (0x1u << 2) /**< \brief (TC_QIER) Quadrature Error */
/* -------- TC_QIDR : (TC Offset: 0xCC) QDEC Interrupt Disable Register -------- */
#define TC_QIDR_IDX (0x1u << 0) /**< \brief (TC_QIDR) Index */
#define TC_QIDR_DIRCHG (0x1u << 1) /**< \brief (TC_QIDR) Direction Change */
#define TC_QIDR_QERR (0x1u << 2) /**< \brief (TC_QIDR) Quadrature Error */
/* -------- TC_QIMR : (TC Offset: 0xD0) QDEC Interrupt Mask Register -------- */
#define TC_QIMR_IDX (0x1u << 0) /**< \brief (TC_QIMR) Index */
#define TC_QIMR_DIRCHG (0x1u << 1) /**< \brief (TC_QIMR) Direction Change */
#define TC_QIMR_QERR (0x1u << 2) /**< \brief (TC_QIMR) Quadrature Error */
/* -------- TC_QISR : (TC Offset: 0xD4) QDEC Interrupt Status Register -------- */
#define TC_QISR_IDX (0x1u << 0) /**< \brief (TC_QISR) Index */
#define TC_QISR_DIRCHG (0x1u << 1) /**< \brief (TC_QISR) Direction Change */
#define TC_QISR_QERR (0x1u << 2) /**< \brief (TC_QISR) Quadrature Error */
#define TC_QISR_MPE (0x1u << 3) /**< \brief (TC_QISR) Consecutive Missing Pulse Error */
#define TC_QISR_DIR (0x1u << 8) /**< \brief (TC_QISR) Direction */
/* -------- TC_FMR : (TC Offset: 0xD8) Fault Mode Register -------- */
#define TC_FMR_ENCF0 (0x1u << 0) /**< \brief (TC_FMR) Enable Compare Fault Channel 0 */
#define TC_FMR_ENCF1 (0x1u << 1) /**< \brief (TC_FMR) Enable Compare Fault Channel 1 */
/* -------- TC_WPMR : (TC Offset: 0xE4) Write Protection Mode Register -------- */
#define TC_WPMR_WPEN (0x1u << 0) /**< \brief (TC_WPMR) Write Protection Enable */
#define TC_WPMR_WPKEY_Pos 8
#define TC_WPMR_WPKEY_Msk (0xffffffu << TC_WPMR_WPKEY_Pos) /**< \brief (TC_WPMR) Write Protection Key */
#define TC_WPMR_WPKEY(value) ((TC_WPMR_WPKEY_Msk & ((value) << TC_WPMR_WPKEY_Pos)))
#define   TC_WPMR_WPKEY_PASSWD (0x54494Du << 8) /**< \brief (TC_WPMR) Writing any other value in this field aborts the write operation of the WPEN bit.Always reads as 0. */


#define OFFSET_TC_CCR	0x0		/* Channel Control Register */
#define OFFSET_TC_CMR	0x4		/* Channel Mode Register */
#define OFFSET_TC_SMMR	0x8		/* Stepper Motor Mode Register */
#define OFFSET_TC_RAB	0xC		/* Register AB */
#define OFFSET_TC_CV	0x10	/* Counter Value */
#define OFFSET_TC_RA	0x14	/* Register A */
#define OFFSET_TC_RB	0x18	/* Register B */
#define OFFSET_TC_RC	0x1C	/* Register C */
#define OFFSET_TC_SR	0x20	/* Interrupt Status Register */
#define OFFSET_TC_IER	0x24	/* Interrupt Enable Register */
#define OFFSET_TC_IDR	0x28	/* Interrupt Disable Register */
#define OFFSET_TC_IMR	0x2C	/* Interrupt Mask Register */
#define OFFSET_TC_EMR	0x30	/* Extended Mode Register */
#define OFFSET_TC_CSR	0x34	/* Channel Status Register */
#define OFFSET_TC_SSR	0x38	/* Safety Status Register */

#define OFFSET_TC_BCR                       0xC0 /* Block Control Register */
#define OFFSET_TC_BMR                      0xC4 /* Block Mode Register */
#define OFFSET_TC_QIER                     0xC8 /* QDEC Interrupt Enable Register */
#define OFFSET_TC_QIDR                      0xCC /* QDEC Interrupt Disable Register */
#define OFFSET_TC_QIMR                     0xD0 /* QDEC Interrupt Mask Register */
#define OFFSET_TC_QISR                      0xD4 /* QDEC Interrupt Status Register */
#define OFFSET_TC_FMR                       0xD8 /* Fault Mode Register */
#define OFFSET_TC_QSR                      0xDC /* QDEC Status Register */
#define OFFSET_TC_WPMR                      0xE4 /* Write Protection Mode Register */




struct capture_data {
	void __iomem *base;
	u32 opmode;
	struct clk *clk;
	unsigned long clk_rate;
	int irq;
	int capture_done;
	u32 buf_counter;
	u32 frequency;
	u32 duty;
	int state;
};


static DECLARE_WAIT_QUEUE_HEAD(tc_wait);  

#define MAX_BUFF_COUNT 5
#define debug_print printk
#define CHANNEL_0 0

#define TC_RUNNING 1
#define TC_IDLE 0


u32 buf_a[MAX_BUFF_COUNT];
u32 buf_b[MAX_BUFF_COUNT];
// struct capture_data *g_data;



void tc_get_ra_rb_rc(void *base, u32 channel,
	u32 *ra, u32 *rb)
{

	base = base + 0x40*channel;

	if (ra)
		*ra = readl(base + OFFSET_TC_RA);
	if (rb)
		*rb = readl(base + OFFSET_TC_RB);

}

u32 tc_get_status(void *base, u32 channel)
{

	base = base + 0x40*channel;
	
	return readl(base+OFFSET_TC_SR);
}


void tc_stop(void *base,u32 channel)
{
	base = base + 0x40*channel;

	writel(TC_CCR_CLKDIS,base + OFFSET_TC_CCR);
}


void tc_start(void *base,u32 channel)
{

	base = base + 0x40*channel;

	/*  Clear status register */
	readl(base + OFFSET_TC_SR);
	writel(TC_CCR_CLKEN | TC_CCR_SWTRG,base + OFFSET_TC_CCR ) ;
}



static int _tcd_capture_polling(void *base)
{
	u32 i;

	for (i = 0; i < MAX_BUFF_COUNT; i ++) {
		while ((tc_get_status(base, CHANNEL_0) & TC_SR_LDRBS) != TC_SR_LDRBS);
		tc_get_ra_rb_rc(base,CHANNEL_0, &buf_a[i], &buf_b[i]);
		printk("a %u, b %u\n",buf_a[i],buf_b[i]);
	}

	
	//tc_stop(desc->addr, desc->channel);
	return 0;
}


void tc_configure(void *base, u32 channel, u32 mode)
{

	base = base + 0x40*channel;

	/*  Disable TC clock */
	writel(TC_CCR_CLKDIS,base + OFFSET_TC_CCR);

	/*  Disable interrupts */
	writel(0x4FF,base + OFFSET_TC_IDR);

	/*  Clear status register */
	readl(base + OFFSET_TC_SR);

	/*  Set mode */
	writel(mode,base + OFFSET_TC_CMR);
}





void tc_init(void *base)
{
	u32 config;
	// mck1 / 8
	// TIOAx is used as an external trigger.
	config = TC_CMR_TCCLKS_TIMER_CLOCK2 | TC_CMR_LDRA_RISING | TC_CMR_LDRB_FALLING | TC_CMR_ABETRG | TC_CMR_ETRGEDG_FALLING;
	tc_configure(base, CHANNEL_0, config);

	//tc_start(base,CHANNEL_0);
}

void tc_disable_irq(void *base,u32 channel)
{
	base = base + 0x40*channel;

	/*  Disable interrupts */
	writel(0x4FF,base + OFFSET_TC_IDR);
	/*  Clear status register */
	readl(base + OFFSET_TC_SR);

}


void tc_enable_irq(void *base,u32 channel)
{
	base = base + 0x40*channel;

	/*  Disable interrupts */
	writel(0x4FF,base + OFFSET_TC_IDR);
	/*  Clear status register */
	readl(base + OFFSET_TC_SR);

	// enable load RB irq
	writel(TC_SR_LDRBS,base + OFFSET_TC_IER);
}


static irqreturn_t tc_interrupt(int irq, void *private)
{
	struct capture_data *ddata = private;
	u32 stat;


	stat = tc_get_status(ddata->base, CHANNEL_0);
	if(stat & TC_SR_LDRBS){
		
		tc_get_ra_rb_rc(ddata->base, CHANNEL_0, &buf_a[ddata->buf_counter], &buf_b[ddata->buf_counter]);
		
		ddata->buf_counter++;
		if(ddata->buf_counter >= MAX_BUFF_COUNT){

			tc_stop(ddata->base, CHANNEL_0);
			tc_disable_irq(ddata->base, CHANNEL_0);
			ddata->capture_done = 1;
			ddata->buf_counter = 0;
			ddata->state = TC_IDLE;

			//debug_print("tc irq done, stat 0x%x\n",stat);
			wake_up(&tc_wait);
		}
	}
	
	return IRQ_HANDLED;
}







#if 0
static struct hrtimer timer;


ktime_t star;
ktime_t end;
ktime_t kt;

static enum hrtimer_restart hrtimer_handler(struct hrtimer *timer)
{
	
	end = timer->base->get_time();
	//hrtimer_forward(timer, now, kt);
	printk("\nhr end\n");
	printk("start time : %lld ns\n",star);
	printk("end time   : %lld ns\n",end);
	
	//return HRTIMER_RESTART;
	return HRTIMER_NORESTART;
}

void hr_timer_test(void)
{
	
	
	kt = ktime_set(0, 1000*1000*1000); /* 1 sec */
	hrtimer_init(&timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	timer.function = hrtimer_handler;
	
	hrtimer_start(&timer, kt, HRTIMER_MODE_REL);
	printk("hr start\n");

	star = timer.base->get_time();
	//printk("start time : %lld ns\n",now.tv64);
		 
}

#endif

static ssize_t sys_read_frequency(struct device *dev, struct device_attribute *attr,char *buf)
{
	struct platform_device *pdev;
	struct capture_data *ddata;

	pdev = container_of(dev, struct platform_device, dev);
	ddata = platform_get_drvdata(pdev);
	
	return scnprintf(buf, PAGE_SIZE, "%d\n",ddata->frequency);

 } 
 

static ssize_t sys_read_duty(struct device *dev, struct device_attribute *attr,char *buf)
{
	struct platform_device *pdev;
	struct capture_data *ddata;

	pdev = container_of(dev, struct platform_device, dev);
	ddata = platform_get_drvdata(pdev);


	return scnprintf(buf, PAGE_SIZE, "%d\n", ddata->duty);
 
 } 
 


static ssize_t sys_write_trigger(struct device *dev, struct device_attribute *attr,const char *buf, size_t count) 
 { 
	int i;
	int ret;
	struct platform_device *pdev;
	struct capture_data *ddata;
	u32 rb, ra;


	pdev = container_of(dev, struct platform_device, dev);
	ddata = platform_get_drvdata(pdev);

	if(*buf =='1' ){
		tc_disable_irq(ddata->base, CHANNEL_0);
		tc_start(ddata->base,CHANNEL_0);
		
		_tcd_capture_polling(ddata->base);

		
		printk("frequency is %u, pulse is %u\n",(u32)ddata->clk_rate / 8 / buf_b[4], (buf_b[4] - buf_a[4]) * 1000 / buf_b[4]);
		tc_stop(ddata->base,CHANNEL_0);
	}
	else if(*buf == '2'){

		if(ddata->state != TC_RUNNING){
			tc_stop(ddata->base,CHANNEL_0);
			tc_enable_irq(ddata->base,CHANNEL_0);
			tc_start(ddata->base,CHANNEL_0);

			ddata->capture_done = 0;
			ddata->buf_counter = 0;
			ddata->state = TC_RUNNING;
			ddata->frequency = 0;
			ddata->duty = 0;
		}

		ret = wait_event_interruptible_timeout(tc_wait,
			ddata->capture_done == 1, HZ*5 );
		
		
		// ddata->state = TC_IDLE;
		if(!ret)
			printk("time out\n");
		else{
		
			rb = 0;
			ra = 0;
			
			// the firt caputre value is unstable,ignore it
			for (i = 1; i < MAX_BUFF_COUNT; i ++) {
				debug_print("a %u, b %u\n",buf_a[i],buf_b[i]);
				rb += buf_b[i];
				ra += buf_a[i];	
			}	
			
			rb = rb / (MAX_BUFF_COUNT -1);
			ra = ra / (MAX_BUFF_COUNT -1);
			
			ddata->duty = (rb - ra) * 1000 / rb;
			ddata->frequency = ddata->clk_rate / 8 / rb;
			
			debug_print("capture done,frequency is %d, pulse is %d\n",ddata->frequency ,ddata->duty);
		}
		
		
	}
	else if(*buf == '3'){
		
		// hr_timer_test();
	}

	
    return count;
 } 



static DEVICE_ATTR(trigger, S_IRUGO | S_IWUSR,NULL,sys_write_trigger);
static DEVICE_ATTR(frequency, S_IRUGO | S_IRUSR,sys_read_frequency,NULL);
static DEVICE_ATTR(duty, S_IRUGO | S_IRUSR,sys_read_duty,NULL);
 
static struct attribute *tc_capture_attrs[]={
	&dev_attr_trigger.attr,
	&dev_attr_frequency.attr,
	&dev_attr_duty.attr,
	NULL,
};




struct attribute_group tc_group={
	.attrs = tc_capture_attrs,
	.name = "tc_capture",
};

static int tc_capture_probe(struct platform_device *pdev)
{
	//struct device_node *np = pdev->dev.of_node;
	struct resource *res;
	struct capture_data *ddata;
	int err,ret;
	unsigned long clk_rate;

	debug_print("\ntc_capture_probe\n");

	
	ddata = devm_kzalloc(&pdev->dev, sizeof(*ddata), GFP_KERNEL);
	if (!ddata)
		return -ENOMEM;

	platform_set_drvdata(pdev, ddata);
	

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	ddata->base = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(ddata->base))
		return PTR_ERR(ddata->base);

	ddata->irq = platform_get_irq(pdev, 0);
	if (ddata->irq < 0)
		return -ENODEV;

	

	ddata->clk = devm_clk_get(&pdev->dev, NULL);
	if (IS_ERR(ddata->clk))
		return PTR_ERR(ddata->clk);



	ddata->clk_rate = clk_rate = clk_get_rate(ddata->clk);
	debug_print("get clk rate %ld\n",clk_rate);
	
	err = clk_prepare_enable(ddata->clk);
	if (err)
		return err;


	tc_init(ddata->base);



	ret = request_irq(ddata->irq, tc_interrupt, 0,
			  pdev->dev.driver->name, ddata);
	if (ret) {
		dev_err(&pdev->dev, "Failed to allocate IRQ.\n");
		return ret;
	}	


	if(sysfs_create_group(&pdev->dev.kobj, &tc_group) != 0)
	{
		printk("create %s sys-file err \n",tc_group.name);
	}
	//g_data = ddata;


	return 0;

}



static int tc_capture_remove(struct platform_device *pdev)
{
	struct capture_data *ddata = platform_get_drvdata(pdev);
	int ret = 0;

	tc_stop(ddata->base,CHANNEL_0);
	tc_disable_irq(ddata->base,CHANNEL_0);
	free_irq(ddata->irq,ddata);
		
	clk_disable_unprepare(ddata->clk);
	sysfs_remove_group(&pdev->dev.kobj, &tc_group);
	
	return ret;
}

static const struct of_device_id tc_capture_of_match[] = {
	{ .compatible = "microchip,tc-capture" },
	{ .compatible = "sama7g54,tc-capture" },
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, tc_capture_of_match);



static struct platform_driver tc_capture_driver = {
	.probe	= tc_capture_probe,
	.remove		= tc_capture_remove,
	.driver	= {
		.name		= "microchip_tc_capure",
		.of_match_table	= tc_capture_of_match,
	},
};

module_platform_driver(tc_capture_driver);

MODULE_AUTHOR("murphy.xu");
MODULE_DESCRIPTION("microchip tc capture driver");
MODULE_LICENSE("GPL v2");


