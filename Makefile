TERMINFODIR=/usr/local/lib/lcdfm/terminfo

install : 
	mkdir -p $(TERMINFODIR)
	tic -o $(TERMINFODIR) terminfo.src
