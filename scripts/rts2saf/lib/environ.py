import time
import datetime
import glob
import os

class Environment():
    """Class performing various task on files, e.g. expansion to (full) path"""
    def __init__(self, rtc=None, log=None):
        self.now= datetime.datetime.now().isoformat()
        self.rtc=rtc
        self.log=log
        self.runTimePath= '{0}/{1}/'.format(self.rtc.cf['BASE_DIRECTORY'], self.now) 
        self.runDateTime=None

    def prefix( self, fileName):
        return 'rts2af-' +fileName

    def notFits(self, fileName):
        items= fileName.split('/')[-1]
        return items.split('.fits')[0]

    def fitsFilesInRunTimePath( self):
        return glob.glob( self.runTimePath + '/' + self.rtc.value('FILE_GLOB'))

    def expandToTmp( self, fileName=None):
        if self.absolutePath(fileName):
            self.log.error('Environment.expandToTmp: absolute path given: {0}'.format(fileName))
            return fileName

        fileName= self.rtc.value('TEMP_DIRECTORY') + '/'+ fileName
        return fileName

    def expandToSkyList( self, fitsHDU=None):
        if fitsHDU==None:
            self.log.error('Environment.expandToCat: no hdu given')
        
        items= self.rtc.value('SEXSKY_LIST').split('.')
        try:
            fileName= self.prefix( items[0] +  '-' + fitsHDU.staticHeaderElements['FILTER'] + '-' +   self.now + '.' + items[1])
        except:
            fileName= self.prefix( items[0] + '-' +   self.now + '.' + items[1])
            
        return  self.expandToTmp(fileName)

    def expandToCat( self, fitsHDU=None):
        if fitsHDU==None:
            self.log.error('Environment.expandToCat: no hdu given')
        try:
            fileName= self.prefix( self.notFits(fitsHDU.fitsFileName) + '-' + fitsHDU.staticHeaderElements['FILTER'] + '-' + self.now + '.cat')
        except:
            fileName= self.prefix( self.notFits(fitsHDU.fitsFileName) + '-' + self.now + '.cat')

        return self.expandToTmp(fileName)

    def expandToFitInput(self, fitsHDU=None, element=None):
        items= self.rtc.value('FIT_RESULT_FILE').split('.')
        if(fitsHDU==None) or ( element==None):
            self.log.error('Environment.expandToFitInput: no hdu or elementgiven')

        fileName= items[0] + "-" + fitsHDU.staticHeaderElements['FILTER'] + "-" + self.now +  "-" + element + "." + items[1]
        return self.expandToTmp(self.prefix(fileName))

    def expandToFitImage(self, fitsHDU=None):
        if fitsHDU==None:
            self.log.error('Environment.expandToFitImage: no hdu given')

        items= self.rtc.value('FIT_RESULT_FILE').split('.') 
# ToDo png
        fileName= items[0] + "-" + fitsHDU.staticHeaderElements['FILTER'] + "-" + self.now + ".png"
        return self.expandToTmp(self.prefix(fileName))

    def absolutePath(self, fileName=None):
        if fileName==None:
            self.log.error('Environment.absolutePath: no file name given')
            
        pLeadingSlash = re.compile( r'\/.*')
        leadingSlash = pLeadingSlash.match(fileName)
        if leadingSlash:
            return True
        else:
            return False

    def defineRunTimePath(self, fileName=None):
        for root, dirs, names in os.walk(self.rtc.value('BASE_DIRECTORY')):
            if fileName.rstrip() in names:
                self.runTimePath= root
                return True
        else:
            self.log.error('Environment.defineRunTimePath: file not found: {0}'.format(fileName))

        return False

    def expandToRunTimePath(self, pathName=None):
        if self.absolutePath(pathName):
            self.runDateTime= pathName.split('/')[-3] # it is rts2af, which creates the directory tree
            return pathName
        else:
            fileName= pathName.split('/')[-1]
            if self.defineRunTimePath( fileName):
                self.runDateTime= self.runTimePath.split('/')[-2]
 
                return self.runTimePath + '/' + fileName
            else:
                return None

# ToDo: refactor with expandToSkyList
    def expandToDs9RegionFileName( self, fitsHDU=None):
        if fitsHDU==None:
            self.log.error('Environment.expandToDs9RegionFileName: no hdu given')
        
        
        items= self.rtc.value('DS9_REGION_FILE').split('.')
        # not nice
        names= fitsHDU.fitsFileName.split('/')
        try:
            fileName= self.prefix( items[0] +  '-' + fitsHDU.staticHeaderElements['FILTER'] + '-' + self.now + '-' + names[-1] + '-' + str(fitsHDU.variableHeaderElements['FOC_POS']) + '.' + items[1])
        except:
            fileName= self.prefix( items[0] + '-' +   self.now + '.' + items[1])
            
        return  self.expandToTmp(fileName)

    def expandToDs9CommandFileName( self, fitsHDU=None):
        if fitsHDU==None:
            self.log.error('Environment.expandToDs9COmmandFileName: no hdu given')
        
        items= self.rtc.value('DS9_REGION_FILE').split('.')
        try:
            fileName= self.prefix( items[0] +  '-' + fitsHDU.staticHeaderElements['FILTER'] + '-' + self.now + '.' + items[1] + '.sh')
        except:
            fileName= self.prefix( items[0] + '-' +   self.now + '.' + items[1]+ '.sh')
            
        return  self.expandToTmp(fileName)
        
    def expandToAcquisitionBasePath(self, filter=None):

        if filter== None:
            return self.rtc.value('BASE_DIRECTORY') + '/' + self.now + '/'  
        else: 
            return self.rtc.value('BASE_DIRECTORY') + '/' + self.now + '/' + filter.name + '/'
        
    def expandToTmpConfigurationPath(self, fileName=None):
        if fileName==None:
            self.log.error('Environment.expandToTmpConfigurationPath: no filename given')

        fileName= fileName + self.now + '.cfg'
        return self.expandToTmp(fileName)

    def expandToFitResultPath(self, fileName=None):
        if fileName==None:
            self.log.error('Environment.expandToFitResultPath: no filename given')

        return self.expandToTmp(fileName + self.now + '.fit')

    def createAcquisitionBasePath(self, filter=None):
        pth= self.expandToAcquisitionBasePath( filter)
        try:
            os.makedirs( pth)
        except:
             self.log.error('Environment.createAcquisitionBasePath failed for {0}'.format(pth))
             return False
        return True
    def setModeExecutable(self, path):
        #mode = os.stat(path)
        os.chmod(path, 0744)
