EPS = central-server-messages.eps 	\
      client-server-direct.eps 		\
      client-server-central.eps 	\
      client-server-central-direct.eps 	\
      kill-nextexe.eps 			\
      kill-pre.eps 			\
      kill-waitexe.eps 			\
      kill-waitblock.eps 		\
      rts-1-packages.eps 		\
      vrstvy-architektura.eps 

all:${EPS}

%.eps: %.dia
	dia --nosplash --export=$@ $<

clean: 
	$(RM) *.eps *~
