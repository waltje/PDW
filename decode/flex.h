#ifndef PDW_FLEX_H
# define PDW_FLEX_H


class FLEX {
    private:
	void	FlexTIME();
	int	xsumchk(long int l);
	void	show_address(long int l, long int l2, bool bLongAddress);
	void	showframe(int asa, int vsa);
	void	show_phase_speed(int vt);

    public:
	char	block[256];		// was 300
	long	frame[200];
	char	ppp;

	FLEX();
	~FLEX();

	void	showblock(int blknum);
	void	showword(int wordnum);
	void	showwordhex(int wordnum);
};


extern int FLEX_9;


#endif	/*PDW_FLEX_H*/
