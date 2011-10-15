# Downloads sections of 100million digits of pi from internet
import urllib,time
for i in xrange(1,1000+1):
    t0=time.time()
    s='http://ja0hxv.calico.jp/value/pi0/pi-100b/pi-%.04d.zip' % i
    fd=urllib.urlretrieve(s,'pi-%.04d.zip'%i)
    print s,time.time()-t0
    
