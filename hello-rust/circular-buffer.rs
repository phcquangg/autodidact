#[derive(Debug)]

pub struct CircularBuffer<T, const N:usize> {
	buffer: [Option<T>; N],
	head: usize,
	tail: usize,
	count: usize,
}

impl<T, const N: usize> CircularBuffer<T, N> {
	pub fn new() -> Self {
		assert!(N > 0 && (N & (N - 1) == 0), "Capacity N must be a power of 2");

		Self {
			buffer: std::array::from_fn(|_| None),
			head: 0,
			tail: 0,
			count: 0,
		}
	}

	pub fn is_full(&self) -> bool {
		self.count == N
	}

	pub fn is_empty(&self) -> bool {
		self.count == 0
	}

	pub fn push(&mut self, item: T) -> Result<(), T> {
		if self.is_full() {
			return Err(item);
		}

		self.buffer[self.head] = Some(item);
		self.head = (self.head + 1) & (N - 1);

		self.count += 1;
		Ok(())
	}

	pub fn pop(&mut self) -> Option<T> {
		if self.is_empty() {
			return None;
		}

		let item = self.buffer[self.tail].take();
		self.tail = (self.tail + 1) & (N - 1);
		self.count -= 1;

		item
	}
}

fn main() {
	let mut cb = CircularBuffer::<i32, 8>::new();

	cb.push(12).unwrap();
	cb.push(24).unwrap();
	cb.push(32).unwrap();

	while let Some(val) = cb.pop() {
		println!("current: {:?}", cb);
		println!("Popped: {}", val);
	}
}

