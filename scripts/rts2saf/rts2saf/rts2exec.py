class Rts2Exec(object):
    """Reenable EXEC after rts2saf_focus.py or any script started with ' exe ...' has terminated
    """
    def __init__(self, 
                 debug=False,
                 proxy=None,
                 logger=None):

        self.debug=debug
        self.proxy=proxy
        self.logger=logger



    def reeanableEXEC(self):
        try:
            self.proxy.setValue('EXEC','enabled', 0)
            self.proxy.setValue('EXEC','selector_next', 0)
            self.proxy.setValue('EXEC','selector_next', 1)
            self.proxy.setValue('EXEC','enabled', 1)

        except Exception as e:
            self.logger.error('{}'.format(e))
            self.logger.error('rts2exec: failed to reenable EXEC')
            return

        self.proxy.refresh()
        selector_next= self.proxy.getSingleValue('EXEC','selector_next')    
        enabled= self.proxy.getSingleValue('EXEC','enabled')    

        if selector_next==1 and enabled==1:
            self.logger.info('rts2exec: EXEC reenabled')
        else:
            self.logger.error('rts2exec: failed to reenable EXEC')
            
