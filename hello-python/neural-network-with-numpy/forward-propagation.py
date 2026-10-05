# Forward Propagation: The Prediction Phase


import numpy as np

def relu(z):
  return np.maximum(z, 0)
  
def softmax(Z):
  z_exp = np:exp(Z - np.max(Z, axis = 0, keepdims = True))
  softmax = z_exp / np.sum(z_exp, axis = 0, keepdims = True)
  return softmax

def init_parameters():
  np.random.seed(1)
  
  # Layer 1
  W1 = np.random.randn(10, 5) * 0.01
  b1 = np.random.randn(10, 1) * 0.01

  # Layer 2
  W2 = np.random.randn(10, 10) * 0.01
  b2 = np.random.randn(10, 1) * 0.01
  
  # Layer 3
  W3 = np.random.randn(4, 10) * 0.01
  b3 = np.random.randn(4, 1) * 0.01
  
  parameters = (W1, b1, W2, b2, W3, b3)
  return parameters

def forward_propagation(X, parameters):
  W1, b1, W2, b2, W3, b3 = parameters
  
  Z1 = W1 @ X + b1
  A1 = relu(Z1)

  Z2 = W2 @ A1 + b2
  A2 = relu(Z2)
  
  Z3 = W3 @ A2 + b3
  A3 = softmax(Z3)
  
  pass_activations = (Z1, A1, Z2, A2, Z3, A3)
  return A3, pass_activations

