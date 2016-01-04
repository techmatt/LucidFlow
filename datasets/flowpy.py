import sys

fnn=0
path=(r'D:\flowpy\untreatedhela72.007')#OVERWRITE YOUR FILEPATH ON THE GIVEN FILEPATH IN BETWEEN ' '.
f=open(path,'rb')
global fileHeaderver,datam2,plt,Gx1,Gx2,ind,index,s3,outpath,filename2,choice5,Params,loge,Paramsl,datal
global textCell,datam2,datam3,numParams,numValues,lookupNumericData,numpy,fnn
fileHeaderver=f.read(10)
fileHeadertextStart=int(f.read(8))#We are locating the different places from where text starts and ends,data starts and ends etc.
fileHeadertextEnd = int(f.read(8))
fileHeaderdataStart=int(f.read(8))
fileHeaderdataEnd =int(f.read(8))
fileHeaderanalysisStart =int(f.read(8))
fileHeaderanalysisEnd = int(f.read(8))
f.seek(fileHeadertextStart,0)
fileHeadertextData = f.read(fileHeadertextEnd -fileHeadertextStart+1)
textdata=fileHeadertextData.replace(chr(0),'')
textCell=textdata.split(textdata[0])#We are separating 1 string into many depending on where '\' is encountered in the original string.
del textCell[0]
fnn=int(fnn)
fnn=str(fnn+1)
filename="textdata"+fnn+".dat"
FILE=open(filename,"w")#Writing the textdata to a file
FILE.writelines(textCell)        
FILE.close()

def lookupNumericData(array,fieldname):#we want to find the element which is after a paritcular element(like'$P1B)in order to find
            num=len(array)#the value of the particular element(like BIT)
            i=0
            for i in range(0,num):
                    if array[i]==fieldname:
                            ans=array[i+1]
                            break
            return ans
                    
                    
def reshape(array1,numParams,numValues):#(reshape an array)
            num=len(array1)
            array=[None]*(numParams)            
            for i in range(0,numParams):
                    array[i]=[None]*(numValues)
            j=0           
            for i in range(0,numValues):
                    for k in range(0,numParams):
                            array[k][i]=array1[j]
                            j=j+1
            return array

numValues = lookupNumericData(textCell,'$TOT')#Finds the number of values
numParams = lookupNumericData(textCell,'$PAR')#Finds the number of Paramaters

paramValsBits= lookupNumericData(textCell,'$P1B')#Finds the number of BIT

numValues=int(numValues)
numParams=int(numParams)
import numpy
f.seek(fileHeaderdataStart,0)#Go to the part of the file where data starts
data = f.read(numValues*numParams*2)#Read the data.
num=len(data)
j=0
datam=[None]*(num/2)
for i in range(0,num/2):
            datam[i]=data[j]+data[j+1]#2 bytes are joined together.
            j=j+2
for i in range(0,num/2):
            datam[i]=numpy.fromstring(datam[i], dtype=numpy.uint16)#Data is read with integer datatype(16 bit)   
            
#Converting Big Endian to Little Endian
datam = numpy.reshape(datam,(numValues*numParams,1))
datam2=[1]*(num/2)
datam2=datam.byteswap()
datam2 = reshape(datam2,numParams,numValues)
datam2=numpy.transpose(datam2)

#Parameters
Params=[None]*(numParams)
for i in range(1,numParams+1):
            Params[i-1]=lookupNumericData(textCell,'$P%dN'%(i))#Different parameters used are found out from the textheader.
Params = numpy.reshape(Params,(1,numParams))
kf=0
datal=[1]*(numValues*numParams)

#logarithmic data
loge=[0]*(numParams)
for i in range(1,numParams+1):        
        decade=lookupNumericData(textCell,'$P%dE'%(i))
        decade=int(decade[0])
        if decade!=0:
            
            Range=int(lookupNumericData(textCell,'$P%dR'%(i)))

            i=i-1
            m=0
            loge[i]=1
            for j in range(kf*numValues,numValues+kf*numValues):
                datal[j]=float(10**(float((datam2[0][m][i])*float(decade)/float(Range))))
                m=m+1 
            kf=kf+1
            
            
if kf!=0:                        
        datal = reshape(datal,numValues,numParams)
j=0

#Parameters which have log values
Paramsl=[None]*(kf)
for i in range(0,numParams):
        if loge[i]==1:
            Paramsl[j]=Params[0][i]
            Paramsl[j]=Paramsl[j]+'l'        
            j=j+1
#Printing values into the file
    
FILE=open("Data.dat","w")
FILE.write('\t')
for i in range(0,numParams):
                s3=str(Params[0][i])
                FILE.write(s3)           #Printing the different parameters in the file.
                FILE.write('\t')
                if i==numParams-1:
                    for t in range(0,numParams):
                        if loge[t]==1:          #log parameters
                                        s=str(Params[0][t])
                                        FILE.write(s)
                                        FILE.write('(log)')
                                        FILE.write('\t')
FILE.write('\n')
              
        #Printing Data values in the file
for i in range(0,numValues):
                s1=str('\t',)
                FILE.write(s1)
                for j in range(0 ,numParams):
                        s=str(datam2[0][i][j],)
                        FILE.write(s)
                        FILE.write('\t')
                        k=0
                        if j==numParams-1:
                            for t in range(0,numParams):    #log values
                                if loge[t]==1:
                                            k=k+1                                        
                                            s=str(datal[i][k-1],)                               
                                            FILE.write(s)
                                            FILE.write('\t')
                                                                                
                s2='\n'
                FILE.write(s2)
        
FILE.close()
