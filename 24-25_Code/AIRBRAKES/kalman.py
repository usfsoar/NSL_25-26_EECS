# dt = 0.1s, pass in as param from pid? depends on where kalman is called
# or calculate dt?

# only for x-axis, assuming horizontal change is negligible
p00 =  -0.7207
p10 = 0.05836
p01 = 0.2469  
p20 = 0.00494
p11 = -0.3308
p02 = 17.65
p30 = -9.656*10**-7 
p21 = 0.002397
p12 = 0.373
p03 = -22.71

MASS = 40 / 2.205
KALMAN_GAIN = [0,0,0]
MODEL_WEIGHT = 0.6
SENSOR_WEIGHT = 0.4
       
def next_step_predictions(prev_alt, prev_vel, prev_accel, flap_distance, dt):
    if()

    curr_drag = p00 + p10*prev_vel + p01*flap_distance + p20*prev_vel**2 + p11*prev_vel*flap_distance + p02*flap_distance**2 + p30*prev_vel**3 + p21*prev_vel**2*flap_distance + p12*prev_vel*flap_distance**2 + p03*flap_distance**3
    curr_accel = -9.8 + curr_drag / MASS
    curr_vel = prev_vel + prev_accel*dt
    curr_alt = prev_alt + prev_alt*dt + 0.5*prev_accel*(dt**2)
    return curr_alt, curr_vel, curr_accel

# init alt: 
def init_model(init_alt, init_vel, flap_distance, dt):
    
    # start at time of 3.6s
    # take sensor input of vel and alt
    # init call next_step_predictions with these vals, driver

    init_accel = p00 + p10*prev_vel + p01*flap_distance + p20*prev_vel**2 + p11*prev_vel*flap_distance + p02*flap_distance**2 + p30*prev_vel**3 + p21*prev_vel**2*flap_distance + p12*prev_vel*flap_distance**2 + p03*flap_distance**3
    next_step_predictions(init_alt, init_vel, init_accel, flap_distance, dt)


def model(sensor_alt, sensor_vel, init_alt, init_vel, flap_distance):
    # implement weighted average

    model_alt = 
    model_vel = 
    model_accel = 

    weighted_avg_alt = (MODEL_WEIGHT*model_alt + SENSOR_WEIGHT*sensor_alt)/ MODEL_WEIGHT + SENSOR_WEIGHT
    weighted_avg_vel = (MODEL_WEIGHT*model_vel + SENSOR_WEIGHT*sensor_vel)/ MODEL_WEIGHT + SENSOR_WEIGHT
    weighted_avg_accel = (MODEL_WEIGHT*model_accel + SENSOR_WEIGHT*sensor_accel)/ MODEL_WEIGHT + SENSOR_WEIGHT



# get_velocity(), get_altitude(), and get_acceleration()? technically don't need that?
# drag might complicate things bc have to add in transition equation, it becomes another part of the state vector?
# no drag for now??????????????????????????????? what the fuck


    




