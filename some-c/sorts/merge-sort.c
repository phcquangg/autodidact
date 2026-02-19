#include <stdio.h>

int merge_sort(int arr[]);
int merge(int left[], int right[], int left_size, int right_size);

int main(void)
{
  int arr[] = {1, 7, 5, 4, 8, 2, 4, 6};

  printf(merge_sort(arr));
}

int merge(int left[], int right[], int left_size, int right_size)
{
  int i = 0;
  int j = 0;
  int k = 0;

  int result[] = {};

  while (i < left_size && j < right_size)
  {
    if (left[i] < right[j])
    {
      result[k] = left[i];
      k++;
      i++;
    }
    else if (left[i] > right[j])
    {
      result[k] = right[j];
      k++;
      j++;
    }
    else if (left[i] == right[j])
    {
      result[k] = left[i];
      result[k + 1] = right[j];

      i++;
      j++;
      k += 2;
    }
  }

  return result;
}

int merge_sort(int arr[])
{
  const int size = sizeof(arr);

  if (size <= 1)
  {
    return arr;
  }
  else
  {
    int middle = size / 2;
    int left[] = {};
    int right[] = {};

    for (int i = 0; i < middle; ++i)
    {
      left[i] = arr[i];
      right[i] = arr[i + middle];
    };

    const int left_size = sizeof(left);
    const int right_size = sizeof(right);

    int sorted_left[] = merge_sort(left);
    int sorted_right[] = merge_sort(right);

    return merge(sorted_left, sorted_right, left_size, right_size);
  }
}
