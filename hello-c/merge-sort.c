#include <stdio.h>

int merge_sort(int arr[], int left, int right);
void merge(int arr[], int left, int middle, int right);

int main(void) {
	int arr[] = { 1,7,5,4,8,2,4,6 };

	int size = sizeof(arr) / sizeof(arr[0]);
	merge_sort(arr, 0, size - 1);

	for (int i = 0; i < size; ++i) {
		printf("%i ", arr[i]);
	}
}

void merge(int arr[], int left, int middle, int right) {
	int n1 = middle - left + 1;
	int n2 = right - middle;

	int l[n1];
  int r[n2];

	for (int i = 0; i < n1; ++i) l[i] = arr[left + i];
	for (int i = 0; i < n2; ++i) r[i] = arr[middle + 1 + i];

	int i = 0, j = 0, k = left;

	while (i < n1 && j < n2) {
		if (l[i] <= r[j]) {
			arr[k] = l[i];
			i ++;
		} else {
			arr[k] = r[j];
			j++;
		}

		k++;
	}

	while (i < n1) {
		arr[k] = l[i];
		i ++;
		k ++;
	}

	while (j < n2) {
		arr[k] = r[j];
		j ++;
		k ++;
	}
}

int merge_sort(int arr[], int left, int right) {
	if (left < right) {
		int middle = left + (right - left) / 2;

		merge_sort(arr, left, middle);
		merge_sort(arr, middle + 1, right);

		merge(arr, left, middle, right);
	}
}
