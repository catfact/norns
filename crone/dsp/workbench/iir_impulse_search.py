import matplotlib.pyplot as plt
import numpy as np
from scipy.optimize import minimize


def compute_impulse(b, a, n=128):
    """
    compute the impulse response of an Nth-order IIR filter
    internally, a test signal is computed which is zero except for the first M samples (which are unity.)
    :param b: feedforward coefficients
    :param a: feedback coefficients (excluding a0, which is assumed to be unity)
    :param n: length of signal, in samples
    :return: list of values
    """
    x = [0] * n

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
        xn[0] = x[i]
        for q in range(Q - 1):
            yn[p + 1] = yn[q]
        yn[0] = y0
        y[i] = y0
    return y


# test with a simple 1-pole lowpass smoother
c = 0.999
a1 = c * -1
b0 = 1 - c
N = 4096
y = compute_impulse([b0], [a1], N)
# plt.plot(y)
# plt.show()  # looks ok

# exponential decay, for comparison
exp_decay = [None] * N
exp_decay[0] = 1
for i in range(N - 1):
    exp_decay[i + 1] = exp_decay[i] * c


# plt.plot(exp_decay)
# plt.show()  # precisely the same


def mse(a, b):
    """
    returns the mean squared error of two lists
    mut be the same size!
    :param a:
    :param b:
    :return: MSE scalar
    """
    return np.square(np.subtract(np.array(a), np.array(b))).mean()


err = mse(y, exp_decay)
print("exponential decay error: {}".format(err))

log_decay = 1 - np.flip(np.array(exp_decay))
# plt.plot(log_decay)
# plt.show()


# ok, that's the preliminaries, let's get real.
# we will try to fit an Nth-order filter to the log-decay impulse response curve.
# since we normalize a0=1, number of variables is (N*2)
# we'll try this to start
N = 3

# here's the function we'll try to minimize
def log_decay_impulse_error(coeffs):
    """
    :param coeffs: vector of filter coefficients; N feedforward concatenated with N-1 feedback
    :return: mean-squared error of IIR response with inverse-exponential-decay curve
    """
    b = coeffs[0:N]
    a = coeffs[N:]
    impulse = compute_impulse(b, a, len(log_decay))
    return mse(impulse, log_decay)


# initial guess will be coefficients for exp-decay
x0 = [b0, 0, 0, a1, 0, 0]

res = minimize(log_decay_impulse_error, np.array(x0), method='Nelder-Mead')
print(res)
xmin = res.x.tolist()

print(xmin)
impulse = compute_impulse(xmin[0:N], xmin[N:], len(log_decay))
plt.plot(impulse)
plt.show()
