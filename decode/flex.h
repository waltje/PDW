#ifndef PDW_FLEX_H
# define PDW_FLEX_H


class FLEX {
    private:
	void	FlexTIME();
	int	xsumchk(long l);
	void	show_address(long l, long l2, int bLongAddress);
	void	showframe(int asa, int vsa);
	void	show_phase_speed(int vt);

    public:
	char	block[256];
	long	frame[200];
	char	ppp;

	FLEX();
	~FLEX();

	void	showblock(int blknum);
	void	showword(int wordnum);
	void	showwordhex(int wordnum);
};


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
extern void	flex_init(void);


#endif	/*PDW_FLEX_H*/
