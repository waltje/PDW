#ifndef PDW_FLEX_H
# define PDW_FLEX_H


/* Global variables for module. */
extern int	bEmpty_Frame,		// set if FLEX-Frame=EMTPY/ERMES-Batch=0
		bFlexActive,
		bReflex,
		bFlexTIME_detected;	// set if FlexTIME is detected
extern int	FLEX_9;			// set if receiving 9-digit capcodes
extern int	g_sps2;
extern int	flex_timer;
extern int	iCurrentFrame;
extern int	iCurrentCycle;


/* Functions. */
extern void	flex_reset(void);
extern void	flex_init(void);
extern void	flex_input(char gin);


#endif	/*PDW_FLEX_H*/
