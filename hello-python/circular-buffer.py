class CircularBuffer:
	def __init__(self, capacity: int = 8):
		if capacity <= 0 or (capacity & (capacity - 1)) != 0:
			raise ValueError("Capacity must be a power of 2")

		self.capacity = capacity
		self.buffer = [None] * capacity
		self.head = 0
		self.tail = 0
		self.count = 0

	def __str__(self) -> str:
		items = [
			self.buffer[(self.tail +i) & (self.capacity - 1)]
			for i in range(self.count)
		]

		return f"CircularBuffer({items}, count={self.count}/{self.capacity})"

	def __repr__(self) -> str:
		return self.__str__()

	def is_empty(self) -> bool:
		return self.count == 0

	def is_full(self) -> bool:
		return self.count == self.capacity

	def push(self, item) -> bool:
		"""Push an item & return false if full"""
		if self.is_full():
			return False

		self.buffer[self.head] = item
		self.head = (self.head + 1) & (self.capacity - 1)
		self.count += 1

		return True
	
	def pop(self):
		if self.is_empty():
			return False

		item = self.buffer[self.tail]
		self.buffer[self.tail] = None
		self.tail = (self.tail + 1) & (self.capacity - 1)
		self.count -= 1
		return item

	


cb = CircularBuffer();
cb.push(10)
cb.push(12)
cb.push(22)
cb.pop()
cb.push(24)
print(cb)

