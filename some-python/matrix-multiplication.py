# https://leetgpu.com/challenges/matrix-multiplication

import jax
import jax.numpy as jnp

@jax.jit
def solve(A:jax.Array, B:jax.Array, M: int, N: int, K: int) -> jax.Array
	A = A.astype(jnp.float32)
	B = A.astype(jnp.float32)

	C = jnp.matmul(A, B)

	return C
