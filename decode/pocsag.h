#ifndef PDW_POCSAG_H
# define PDW_POCSAG_H


class POCSAG {
    private:
	long	pocaddr;
	int	wordc, nalp, nnum, shown, srca, srcn;
	int	bAddressWord; // Will be set if last word was an address flag
	int	alp[MAX_STR_LEN],
		num[40];
	int	function;

	void	show_addr(int bAlpha);
	void	show_message();
	void	logbits(char *text, int bClose);
	int	GetMessageType(void);

    public:
	POCSAG();
	~POCSAG();

	void	reset(void);
	void	process_word(int fn2);
	void	frame(int bit);
};


/* Global variables for module. */
extern POCSAG	pocsag;
extern int	pocsag_baud_rate,
		pocbit;


#endif	/*PDW_POCSAG_H*/
