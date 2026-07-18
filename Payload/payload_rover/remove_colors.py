import cv2
import numpy as np

irflash = cv2.imread('IRFlash.png')
noflash = cv2.imread('IRnoFlash.png')
b1, g1, r1 = cv2.split(irflash)
b2, g2, r2 = cv2.split(noflash)

emptyChannel = np.zeros_like(b1)
irFlashOutput = cv2.merge((b1, emptyChannel, emptyChannel))
noFlashOutput = cv2.merge((b2, emptyChannel, emptyChannel))

irFlashOutput_noBlue = cv2.merge((emptyChannel, g1, r1))
noFlashOutput_noBlue = cv2.merge((emptyChannel, g2, r2))

cv2.imwrite('IRFlash_blue.png', b1)#irFlashOutput)
cv2.imwrite('IRnoFlash_blue.png', b2)# noFlashOutput) 

cv2.imwrite('IRFlash_noBlue.png', r1) #irFlashOutput_noBlue)
cv2.imwrite('IRnoFlash_noBlue.png', r2) #noFlashOutput_noBlue)

cv2.imwrite('IRFlash_noBlue_g.png', g1) #irFlashOutput_noBlue)
cv2.imwrite('IRnoFlash_noBlue_g.png', g2) #noFlashOutput_noBlue)

difference = np.asmatrix(b1) - np.asmatrix(b2)
onlyIR = cv2.merge((difference, emptyChannel, emptyChannel))

cv2.imwrite('image_difference.png', difference)