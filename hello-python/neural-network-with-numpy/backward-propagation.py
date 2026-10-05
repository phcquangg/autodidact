# Backward propagation : Learning from Errors
import numpy as np

def backward_prop(X, Y, parameters, forwardpass_activations):
  m = X.shape[1]

  W1, b1, W2, b2, W3, b3 = parameters
  Z1, A1, Z2, A2, Z3, A3 = forwardpass_activations
  
  # Layer 3
  dl_dZ3 = A3 - Y
  
  dZ3_dW3 = A2.T
  dl_dW3 = (1 / m) * np.dot(dL_dZ3, dZ3_dW3)

  dz3_db3 = 1
  dL_db3 = (1 / m) * np.sum(dl_dZ3, axis = 1, keepdims = True)

  # Layer 2
  dZ3_dA2 = W3.T
  dL_dA2 = np.dot(dZ3_dA2, dL_dZ3) # switched positions
  
  dA2_dZ2 = relu_derivative(Z2) # here A2 is f(z2)
  dl_dZ2 = dL_dA2 * dA2_dZ2
  
  dZ2_dW2 = A1.T
  dL_dW2 = (1 / m) * np.dot(dL_dZ2, dZ2_dW2)

  dZ2_db2 = 1
  dL_db2 = (1 / m) * np.sum(dL_dZ2, axis = 1, keepdims = True)

  # Layer 1
  dZ2_dA1 = W2.T
  dL_dA1 = np.dot(dZ2_dA1, dL_dZ2) # switched positions
  
  dA1_dZ1 = relu_derivative(Z1) # here A1 is f(z1)
  dl_dZ1 = dL_dA1 * dA1_dZ1
  
  dZ1_dW1 = X.T
  dL_dW1 = (1 / m) * np.dot(dl_dZ1, dZ1_dW1)
  
  dZ1_db1 = 1
  dL_db1 = (1 / m) * np.sum(dl_dZ1, axis = 1, keepdims = True)
  
  pass_gradients = (dL_dW1, dL_db1, dL_dW2, dL_db2, dL_dW3, dL_db3)
  return pass_gradients


def update_parameters(parameters, gradients, learning_rate):
  W1, b1, W2, b2, W3, b3 = parameters
  dL_dW1, dL_db1, dL_dW2, dL_db2, dL_dW3, dL_db3 = gradients
  
  W1 = W1 - learning_rate * dL_dW1
  b1 = b1 - learning_rate * dL_db1
  W2 = W2 - learning_rate * dL_dW2
  b2 = b2 - learning_rate * dL_db2
  W3 = W3 - learning_rate * dL_dW3
  b3 = b3 - learning_rate * dL_db3
  
  updated_parameters = (W1, b1, W2, b2, W3, b3)
  return updated_parameters