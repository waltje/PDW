#ifndef PDW_POCSAG_H
# define PDW_POCSAG_H


class POCSAG {
    private:
	long	pocaddr;
	int	wordc, nalp, nnum, shown, srca, srcn;
	bool	bAddressWord; // Will be set if last word was an address flag
	int	alp[MAX_STR_LEN],
		num[40];
	int	function;

	void	show_addr(bool bAlpha);
	void	show_message();
	void	logbits(char *text, bool bClose);
	int	GetMessageType();

    public:
	POCSAG();
	~POCSAG();

	void	reset();
	void	process_word(int fn2);
	void	frame(int bit);
};


#endif	/*PDW_POCSAG_H*/
