import numpy as np
from filterpy.kalman import KalmanFilter
from filterpy.common import Q_discrete_white_noise

#needs pip install filterpy

# Class to filter out noisy data

#dt = time intervals
class fusedKalmanFilter():

    #dt = time intervals, std_limit = how many std until data is discarded
    def __init__(self, dt=0.1, sd_limit=3.0):

        #dim_x for pos, vel, and acc, dim_z for baro and accel reading
        self.kf = KalmanFilter(dim_x=3, dim_z=2)
        self.sd_limit = sd_limit

        #intial state (alt, vel, and accel = 0)
        self.kf.x = np.array([0., 0., 0.])

        #state transition matrix
        # x = x + v*dt + .5*a*dt^2
        # v = v + a*dt
        # a = a
        self.kf.F = np.array([[1., dt, 0.5*dt**2], [0., 1., dt], [0., 0., 1.]])

        #measurement, first number is alititude and second is acceleration
        self.kf.H = np.array([[1., 0., 0.], [0., 0., 1.]])

        #P, R, and Q need adjusting/testing
        #####################################

        #uncertainty
        self.kf.P *= 100.

        #sensor noise (trust)
        self.kf.R = np.array([[0.5, 0. ], [0., 0.1]])

        #process noise
        self.kf.Q = Q_discrete_white_noise(dim=3, dt=dt, var=0.01)
        ######################################


    def kalman_update(self, baro_val, accel_val):
        self.kf.predict()

        z = np.array([baro_val, accel_val])

        #use mahalanobis distance to compare standard deviation

        #residual
        y = z - (self.kf.H @ self.kf.x)
         # system uncertainty
        S = self.kf.H @ self.kf.P @ self.kf.H.T + self.kf.R
        #mahalanobis distance
        dist = np.sqrt(float(y.T @ np.linalg.inv(S) @ y))
    
        if dist < self.sd_limit:
            self.kf.update(z)
        else:
            print(f"outlier, skipping update.")
            self.kf.update(None)
        
        return self.kf.x



if __name__ == '__main__':
    pass