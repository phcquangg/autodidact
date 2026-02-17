
import tensorflow as tf
import matplotlib.pyplot as plt

# Check for GPU availability [2]
tf.config.list_physical_devices('GPU')

# 2. Load the Dataset [3]
fashion_mnist = tf.keras.datasets.fashion_mnist
(train_images, train_labels), (valid_images, valid_labels) = fashion_mnist.load_data()

# 3. Explore and Visualize Training Data [4, 5]
# The question number to study with. Feel free to change up to 59999.
data_idx = 42
plt.figure()
plt.imshow(train_images[data_idx], cmap='gray')
plt.colorbar()
plt.grid(False)
plt.show()

# Check the label for the training image [5]
print(f"Training Label: {train_labels[data_idx]}")

# 4. Explore Validation Data [5-7]
# The question number to quiz with. Feel free to change up to 9999.
data_idx = 6174
plt.figure()
plt.imshow(valid_images[data_idx], cmap='gray')
plt.colorbar()
plt.grid(False)
plt.show()

# Check the label for the validation image [6]
print(f"Validation Label: {valid_labels[data_idx]}")

# View raw pixel values (28 lists with 28 values each) [7]
# valid_images[data_idx] 

# 5. Define the Model Architecture [8]
number_of_classes = train_labels.max() + 1
model = tf.keras.Sequential([
    tf.keras.layers.Flatten(input_shape=(28, 28)),
    tf.keras.layers.Dense(number_of_classes)
])

# 6. Verify the Model [9, 10]
model.summary()

# Manual verification of parameter counts
image_height = 28
image_width = 28 # Implied by source material calculation [10]
number_of_weights = image_height * image_width * number_of_classes
print(f"Number of weights: {number_of_weights}")

# Plot the model structure
tf.keras.utils.plot_model(model, show_shapes=True)

# 7. Initiate Training (Compile and Fit) [11, 12]
model.compile(optimizer='adam',
              loss=tf.keras.losses.SparseCategoricalCrossentropy(from_logits=True),
              metrics=['accuracy'])

history = model.fit(
    train_images,
    train_labels,
    epochs=5,
    verbose=True,
    validation_data=(valid_images, valid_labels)
)

# 8. Make Predictions and Visualize Results [13, 14]
# Select a new index to test the model
data_idx = 8675 

# Plot the test image
plt.figure()
plt.imshow(train_images[data_idx], cmap='gray')
plt.colorbar()
plt.grid(False)
plt.show()

# Plot the model's confidence for each of the 10 classes
x_values = range(number_of_classes)
plt.figure()
# Note: Keras expects a batch for prediction, so we use a slice [13, 15]
plt.bar(x_values, model.predict(train_images[data_idx:data_idx+1]).flatten())
plt.xticks(range(10))
plt.show()

# Compare against the correct answer
print("correct answer:", train_labels[data_idx])