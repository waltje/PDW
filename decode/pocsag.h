#ifndef PDW_POCSAG_H
# define PDW_POCSAG_H


/* Global variables for module. */
extern int	pocsag_baud_rate,
		pocbit;


/* Functions. */
extern void	pocsag_init(void);
extern void	pocsag_reset(void);
extern void	pocsag_input(char gin);


#endif	/*PDW_POCSAG_H*/
