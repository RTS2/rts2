EPS = bootes.eps			\
      central-server-messages.eps 	\
      client-server-direct.eps 		\
      client-server-central.eps 	\
      client-server-central-direct.eps 	\
      client-server-use1.eps		\
      kill-nextexe.eps 			\
      kill-pre.eps 			\
      kill-waitexe.eps 			\
      kill-waitblock.eps 		\
      rts-1-packages.eps 		\
      vrstvy-architektura.eps 

#     client-server-use2.eps		\
#     client-server-use3.eps		\

all:${EPS}

%.eps: %.dia
		dia --export=$@ $<

clean: 
	$(RM) *.eps *~
