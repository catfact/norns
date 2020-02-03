import matplotlib.pyplot as plt
import numpy as np
from scipy.optimize import minimize

def compute_impulse(b, a, n=128):
    """
    compute the impulse response of an Nth-order IIR filte
    internally we fill the history with ones, and input value is always zero.
    :param b: feedforward coefficients
    :param a: feedback coefficients (excluding a0, which is assumed to be unity)
    :param n: length of signal, in samples
    :return: list of values
    """
    P = len(b)  # feedforward order
    Q = len(a)  # feedback order
    xn = [1] * P  # feedforward history
    yn = [1] * Q  # feedback history

    y = [None] * n  # output signal
    for i in range(n):

        # compute new output
        y0 = 0
        for p in range(P):
            y0 = y0 + b[p] * xn[p]
        for q in range(Q):
            y0 = y0 - a[q] * yn[q]

        # store I/O history
        for p in range(P - 1):
            xn[p + 1] = xn[p]
        xn[0] = 0 # input is always zero, after initial history.
        for q in range(Q - 1):
            yn[p + 1] = yn[q]
        yn[0] = y0
        y[i] = y0
    return y



def tau2pole(tau, sr=48000, ref=-6.9):
    """
    compute pole coefficient for a 1-pole lowpass smoother
    :param tau: time constant
    :param sr: sample rate
    :param ref: target ratio for convergence is defined as e^ref ; default gives -60db
    :return: coefficient
    """
    return np.exp(ref / (tau * sr))


def mse(a, b):
    """
    returns the mean squared error of two lists
    mut be the same size!
    :param a:
    :param b:
    :return: MSE scalar
    """
    return np.square(np.subtract(np.array(a), np.array(b))).mean()

def log_smoother_coeffs(tau, sr=48000):
    """
    a set of 3rd-order filter coefficients approximating log-decay impulse response,
    given convergence time and samplerate
    :param tau: convergence time
    :param sr: sample rate
    :return:
    """
    c = tau2pole(tau, sr)
    print("exponential decay coefficient: {}".format(c))
    nsamps = int(tau * sr) # number of samples we'll need to capture the whole decay tail, with some padding
    a1 = c * -1
    b0 = 1 - c
    exp_decay = compute_impulse([b0], [a1], nsamps)

    plt.plot(exp_decay)
    plt.show()

    log_decay = 1 - np.flip(np.array(exp_decay))

    plt.plot(log_decay)
    plt.show()

    N = 3
    def err_func(coeffs):
        b = coeffs[0:N]
        a = coeffs[N:]
        impulse = compute_impulse(b, a, len(log_decay))
        return mse(impulse, log_decay)

    x0 = [b0, 0, 0, a1, 0, 0]
    print("performing search...")
    res = minimize(err_func, np.array(x0), method='Nelder-Mead')
    print("done")
    print(res)
    coeffs = res.x.tolist()

    b = coeffs[0:N]
    a = coeffs[N:]

    plt.plot(compute_impulse(b, a, len(log_decay)))
    plt.show()
    return coeffs




# breakpoints for time values
t_breaks = np.logspace(-2, 1.0, 20)
c = []

for t in t_breaks:
    print("computing coeffs for time {}".format(t))
    c.append({ 'time': t,'coeffs': log_smoother_coeffs(t)})

for x in c:
    print("{} : {}".format(x['time'], x['coeffs']))