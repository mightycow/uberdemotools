#include "message.hpp"


static const u16 HuffmanDecoderTable[2048] =
{
	2512, 2182, 512, 2763, 1859, 2808, 512, 2360, 1918, 1988, 512, 1803, 2158, 2358, 512, 2180,
	1798, 2053, 512, 1804, 2603, 1288, 512, 2166, 2285, 2167, 512, 1281, 1640, 2767, 512, 1664,
	1731, 2116, 512, 2788, 1791, 1808, 512, 1840, 2153, 1921, 512, 2708, 2723, 1549, 512, 2046,
	1893, 2717, 512, 2602, 1801, 1288, 512, 1568, 2480, 2062, 512, 1281, 2145, 2711, 512, 1543,
	1909, 2150, 512, 2077, 2338, 2762, 512, 2162, 1794, 2024, 512, 2168, 1922, 2447, 512, 2334,
	1857, 2117, 512, 2100, 2240, 1288, 512, 2186, 2321, 1908, 512, 1281, 1640, 2242, 512, 1664,
	1731, 2729, 512, 2633, 1791, 1919, 512, 2184, 1917, 1802, 512, 2710, 1795, 1549, 512, 2172,
	2375, 2789, 512, 2171, 2187, 1288, 512, 1568, 2095, 2163, 512, 1281, 1858, 1923, 512, 1543,
	2374, 2446, 512, 2181, 1859, 2160, 512, 2183, 1918, 1988, 512, 1803, 2161, 2751, 512, 2413,
	1798, 2529, 512, 1804, 2344, 1288, 512, 2404, 2156, 2786, 512, 1281, 1640, 2641, 512, 1664,
	1731, 2052, 512, 2170, 1791, 1808, 512, 1840, 2395, 1921, 512, 2586, 2319, 1549, 512, 2046,
	1893, 2101, 512, 2159, 1801, 1288, 512, 1568, 2247, 2773, 512, 1281, 2365, 2410, 512, 1543,
	1909, 2781, 512, 2097, 2411, 2740, 512, 2396, 1794, 2024, 512, 2734, 1922, 2733, 512, 2112,
	1857, 2528, 512, 2593, 2079, 1288, 512, 2648, 2143, 1908, 512, 1281, 1640, 2770, 512, 1664,
	1731, 2169, 512, 2714, 1791, 1919, 512, 2185, 1917, 1802, 512, 2398, 1795, 1549, 512, 2098,
	2801, 2361, 512, 2400, 2328, 1288, 512, 1568, 2783, 2713, 512, 1281, 1858, 1923, 512, 1543,
	2816, 2182, 512, 2497, 1859, 2397, 512, 2794, 1918, 1988, 512, 1803, 2158, 2772, 512, 2180,
	1798, 2053, 512, 1804, 2464, 1288, 512, 2166, 2285, 2167, 512, 1281, 1640, 2764, 512, 1664,
	1731, 2116, 512, 2620, 1791, 1808, 512, 1840, 2153, 1921, 512, 2716, 2384, 1549, 512, 2046,
	1893, 2448, 512, 2722, 1801, 1288, 512, 1568, 2472, 2062, 512, 1281, 2145, 2376, 512, 1543,
	1909, 2150, 512, 2077, 2366, 2709, 512, 2162, 1794, 2024, 512, 2168, 1922, 2735, 512, 2407,
	1857, 2117, 512, 2100, 2240, 1288, 512, 2186, 2779, 1908, 512, 1281, 1640, 2242, 512, 1664,
	1731, 2359, 512, 2705, 1791, 1919, 512, 2184, 1917, 1802, 512, 2642, 1795, 1549, 512, 2172,
	2394, 2645, 512, 2171, 2187, 1288, 512, 1568, 2095, 2163, 512, 1281, 1858, 1923, 512, 1543,
	2450, 2771, 512, 2181, 1859, 2160, 512, 2183, 1918, 1988, 512, 1803, 2161, 2585, 512, 2403,
	1798, 2619, 512, 1804, 2777, 1288, 512, 2355, 2156, 2362, 512, 1281, 1640, 2380, 512, 1664,
	1731, 2052, 512, 2170, 1791, 1808, 512, 1840, 2811, 1921, 512, 2402, 2601, 1549, 512, 2046,
	1893, 2101, 512, 2159, 1801, 1288, 512, 1568, 2247, 2719, 512, 1281, 2747, 2776, 512, 1543,
	1909, 2725, 512, 2097, 2445, 2765, 512, 2638, 1794, 2024, 512, 2444, 1922, 2774, 512, 2112,
	1857, 2727, 512, 2644, 2079, 1288, 512, 2800, 2143, 1908, 512, 1281, 1640, 2580, 512, 1664,
	1731, 2169, 512, 2646, 1791, 1919, 512, 2185, 1917, 1802, 512, 2588, 1795, 1549, 512, 2098,
	2322, 2504, 512, 2623, 2350, 1288, 512, 1568, 2323, 2721, 512, 1281, 1858, 1923, 512, 1543,
	2512, 2182, 512, 2746, 1859, 2798, 512, 2360, 1918, 1988, 512, 1803, 2158, 2358, 512, 2180,
	1798, 2053, 512, 1804, 2745, 1288, 512, 2166, 2285, 2167, 512, 1281, 1640, 2806, 512, 1664,
	1731, 2116, 512, 2796, 1791, 1808, 512, 1840, 2153, 1921, 512, 2582, 2761, 1549, 512, 2046,
	1893, 2793, 512, 2647, 1801, 1288, 512, 1568, 2480, 2062, 512, 1281, 2145, 2738, 512, 1543,
	1909, 2150, 512, 2077, 2338, 2715, 512, 2162, 1794, 2024, 512, 2168, 1922, 2447, 512, 2334,
	1857, 2117, 512, 2100, 2240, 1288, 512, 2186, 2321, 1908, 512, 1281, 1640, 2242, 512, 1664,
	1731, 2795, 512, 2750, 1791, 1919, 512, 2184, 1917, 1802, 512, 2732, 1795, 1549, 512, 2172,
	2375, 2604, 512, 2171, 2187, 1288, 512, 1568, 2095, 2163, 512, 1281, 1858, 1923, 512, 1543,
	2374, 2446, 512, 2181, 1859, 2160, 512, 2183, 1918, 1988, 512, 1803, 2161, 2813, 512, 2413,
	1798, 2529, 512, 1804, 2344, 1288, 512, 2404, 2156, 2743, 512, 1281, 1640, 2748, 512, 1664,
	1731, 2052, 512, 2170, 1791, 1808, 512, 1840, 2395, 1921, 512, 2637, 2319, 1549, 512, 2046,
	1893, 2101, 512, 2159, 1801, 1288, 512, 1568, 2247, 2812, 512, 1281, 2365, 2410, 512, 1543,
	1909, 2799, 512, 2097, 2411, 2802, 512, 2396, 1794, 2024, 512, 2649, 1922, 2595, 512, 2112,
	1857, 2528, 512, 2790, 2079, 1288, 512, 2634, 2143, 1908, 512, 1281, 1640, 2724, 512, 1664,
	1731, 2169, 512, 2730, 1791, 1919, 512, 2185, 1917, 1802, 512, 2398, 1795, 1549, 512, 2098,
	2605, 2361, 512, 2400, 2328, 1288, 512, 1568, 2787, 2810, 512, 1281, 1858, 1923, 512, 1543,
	2803, 2182, 512, 2497, 1859, 2397, 512, 2758, 1918, 1988, 512, 1803, 2158, 2598, 512, 2180,
	1798, 2053, 512, 1804, 2464, 1288, 512, 2166, 2285, 2167, 512, 1281, 1640, 2726, 512, 1664,
	1731, 2116, 512, 2583, 1791, 1808, 512, 1840, 2153, 1921, 512, 2712, 2384, 1549, 512, 2046,
	1893, 2448, 512, 2639, 1801, 1288, 512, 1568, 2472, 2062, 512, 1281, 2145, 2376, 512, 1543,
	1909, 2150, 512, 2077, 2366, 2731, 512, 2162, 1794, 2024, 512, 2168, 1922, 2766, 512, 2407,
	1857, 2117, 512, 2100, 2240, 1288, 512, 2186, 2809, 1908, 512, 1281, 1640, 2242, 512, 1664,
	1731, 2359, 512, 2587, 1791, 1919, 512, 2184, 1917, 1802, 512, 2643, 1795, 1549, 512, 2172,
	2394, 2635, 512, 2171, 2187, 1288, 512, 1568, 2095, 2163, 512, 1281, 1858, 1923, 512, 1543,
	2450, 2749, 512, 2181, 1859, 2160, 512, 2183, 1918, 1988, 512, 1803, 2161, 2778, 512, 2403,
	1798, 2791, 512, 1804, 2775, 1288, 512, 2355, 2156, 2362, 512, 1281, 1640, 2380, 512, 1664,
	1731, 2052, 512, 2170, 1791, 1808, 512, 1840, 2805, 1921, 512, 2402, 2741, 1549, 512, 2046,
	1893, 2101, 512, 2159, 1801, 1288, 512, 1568, 2247, 2769, 512, 1281, 2739, 2780, 512, 1543,
	1909, 2737, 512, 2097, 2445, 2596, 512, 2757, 1794, 2024, 512, 2444, 1922, 2599, 512, 2112,
	1857, 2804, 512, 2744, 2079, 1288, 512, 2707, 2143, 1908, 512, 1281, 1640, 2782, 512, 1664,
	1731, 2169, 512, 2742, 1791, 1919, 512, 2185, 1917, 1802, 512, 2718, 1795, 1549, 512, 2098,
	2322, 2504, 512, 2581, 2350, 1288, 512, 1568, 2323, 2597, 512, 1281, 1858, 1923, 512, 1543,
	2512, 2182, 512, 2763, 1859, 2808, 512, 2360, 1918, 1988, 512, 1803, 2158, 2358, 512, 2180,
	1798, 2053, 512, 1804, 2603, 1288, 512, 2166, 2285, 2167, 512, 1281, 1640, 2767, 512, 1664,
	1731, 2116, 512, 2788, 1791, 1808, 512, 1840, 2153, 1921, 512, 2708, 2723, 1549, 512, 2046,
	1893, 2717, 512, 2602, 1801, 1288, 512, 1568, 2480, 2062, 512, 1281, 2145, 2711, 512, 1543,
	1909, 2150, 512, 2077, 2338, 2762, 512, 2162, 1794, 2024, 512, 2168, 1922, 2447, 512, 2334,
	1857, 2117, 512, 2100, 2240, 1288, 512, 2186, 2321, 1908, 512, 1281, 1640, 2242, 512, 1664,
	1731, 2729, 512, 2633, 1791, 1919, 512, 2184, 1917, 1802, 512, 2710, 1795, 1549, 512, 2172,
	2375, 2789, 512, 2171, 2187, 1288, 512, 1568, 2095, 2163, 512, 1281, 1858, 1923, 512, 1543,
	2374, 2446, 512, 2181, 1859, 2160, 512, 2183, 1918, 1988, 512, 1803, 2161, 2751, 512, 2413,
	1798, 2529, 512, 1804, 2344, 1288, 512, 2404, 2156, 2786, 512, 1281, 1640, 2641, 512, 1664,
	1731, 2052, 512, 2170, 1791, 1808, 512, 1840, 2395, 1921, 512, 2586, 2319, 1549, 512, 2046,
	1893, 2101, 512, 2159, 1801, 1288, 512, 1568, 2247, 2773, 512, 1281, 2365, 2410, 512, 1543,
	1909, 2781, 512, 2097, 2411, 2740, 512, 2396, 1794, 2024, 512, 2734, 1922, 2733, 512, 2112,
	1857, 2528, 512, 2593, 2079, 1288, 512, 2648, 2143, 1908, 512, 1281, 1640, 2770, 512, 1664,
	1731, 2169, 512, 2714, 1791, 1919, 512, 2185, 1917, 1802, 512, 2398, 1795, 1549, 512, 2098,
	2801, 2361, 512, 2400, 2328, 1288, 512, 1568, 2783, 2713, 512, 1281, 1858, 1923, 512, 1543,
	3063, 2182, 512, 2497, 1859, 2397, 512, 2794, 1918, 1988, 512, 1803, 2158, 2772, 512, 2180,
	1798, 2053, 512, 1804, 2464, 1288, 512, 2166, 2285, 2167, 512, 1281, 1640, 2764, 512, 1664,
	1731, 2116, 512, 2620, 1791, 1808, 512, 1840, 2153, 1921, 512, 2716, 2384, 1549, 512, 2046,
	1893, 2448, 512, 2722, 1801, 1288, 512, 1568, 2472, 2062, 512, 1281, 2145, 2376, 512, 1543,
	1909, 2150, 512, 2077, 2366, 2709, 512, 2162, 1794, 2024, 512, 2168, 1922, 2735, 512, 2407,
	1857, 2117, 512, 2100, 2240, 1288, 512, 2186, 2779, 1908, 512, 1281, 1640, 2242, 512, 1664,
	1731, 2359, 512, 2705, 1791, 1919, 512, 2184, 1917, 1802, 512, 2642, 1795, 1549, 512, 2172,
	2394, 2645, 512, 2171, 2187, 1288, 512, 1568, 2095, 2163, 512, 1281, 1858, 1923, 512, 1543,
	2450, 2771, 512, 2181, 1859, 2160, 512, 2183, 1918, 1988, 512, 1803, 2161, 2585, 512, 2403,
	1798, 2619, 512, 1804, 2777, 1288, 512, 2355, 2156, 2362, 512, 1281, 1640, 2380, 512, 1664,
	1731, 2052, 512, 2170, 1791, 1808, 512, 1840, 2811, 1921, 512, 2402, 2601, 1549, 512, 2046,
	1893, 2101, 512, 2159, 1801, 1288, 512, 1568, 2247, 2719, 512, 1281, 2747, 2776, 512, 1543,
	1909, 2725, 512, 2097, 2445, 2765, 512, 2638, 1794, 2024, 512, 2444, 1922, 2774, 512, 2112,
	1857, 2727, 512, 2644, 2079, 1288, 512, 2800, 2143, 1908, 512, 1281, 1640, 2580, 512, 1664,
	1731, 2169, 512, 2646, 1791, 1919, 512, 2185, 1917, 1802, 512, 2588, 1795, 1549, 512, 2098,
	2322, 2504, 512, 2623, 2350, 1288, 512, 1568, 2323, 2721, 512, 1281, 1858, 1923, 512, 1543,
	2512, 2182, 512, 2746, 1859, 2798, 512, 2360, 1918, 1988, 512, 1803, 2158, 2358, 512, 2180,
	1798, 2053, 512, 1804, 2745, 1288, 512, 2166, 2285, 2167, 512, 1281, 1640, 2806, 512, 1664,
	1731, 2116, 512, 2796, 1791, 1808, 512, 1840, 2153, 1921, 512, 2582, 2761, 1549, 512, 2046,
	1893, 2793, 512, 2647, 1801, 1288, 512, 1568, 2480, 2062, 512, 1281, 2145, 2738, 512, 1543,
	1909, 2150, 512, 2077, 2338, 2715, 512, 2162, 1794, 2024, 512, 2168, 1922, 2447, 512, 2334,
	1857, 2117, 512, 2100, 2240, 1288, 512, 2186, 2321, 1908, 512, 1281, 1640, 2242, 512, 1664,
	1731, 2795, 512, 2750, 1791, 1919, 512, 2184, 1917, 1802, 512, 2732, 1795, 1549, 512, 2172,
	2375, 2604, 512, 2171, 2187, 1288, 512, 1568, 2095, 2163, 512, 1281, 1858, 1923, 512, 1543,
	2374, 2446, 512, 2181, 1859, 2160, 512, 2183, 1918, 1988, 512, 1803, 2161, 2813, 512, 2413,
	1798, 2529, 512, 1804, 2344, 1288, 512, 2404, 2156, 2743, 512, 1281, 1640, 2748, 512, 1664,
	1731, 2052, 512, 2170, 1791, 1808, 512, 1840, 2395, 1921, 512, 2637, 2319, 1549, 512, 2046,
	1893, 2101, 512, 2159, 1801, 1288, 512, 1568, 2247, 2812, 512, 1281, 2365, 2410, 512, 1543,
	1909, 2799, 512, 2097, 2411, 2802, 512, 2396, 1794, 2024, 512, 2649, 1922, 2595, 512, 2112,
	1857, 2528, 512, 2790, 2079, 1288, 512, 2634, 2143, 1908, 512, 1281, 1640, 2724, 512, 1664,
	1731, 2169, 512, 2730, 1791, 1919, 512, 2185, 1917, 1802, 512, 2398, 1795, 1549, 512, 2098,
	2605, 2361, 512, 2400, 2328, 1288, 512, 1568, 2787, 2810, 512, 1281, 1858, 1923, 512, 1543,
	2803, 2182, 512, 2497, 1859, 2397, 512, 2758, 1918, 1988, 512, 1803, 2158, 2598, 512, 2180,
	1798, 2053, 512, 1804, 2464, 1288, 512, 2166, 2285, 2167, 512, 1281, 1640, 2726, 512, 1664,
	1731, 2116, 512, 2583, 1791, 1808, 512, 1840, 2153, 1921, 512, 2712, 2384, 1549, 512, 2046,
	1893, 2448, 512, 2639, 1801, 1288, 512, 1568, 2472, 2062, 512, 1281, 2145, 2376, 512, 1543,
	1909, 2150, 512, 2077, 2366, 2731, 512, 2162, 1794, 2024, 512, 2168, 1922, 2766, 512, 2407,
	1857, 2117, 512, 2100, 2240, 1288, 512, 2186, 2809, 1908, 512, 1281, 1640, 2242, 512, 1664,
	1731, 2359, 512, 2587, 1791, 1919, 512, 2184, 1917, 1802, 512, 2643, 1795, 1549, 512, 2172,
	2394, 2635, 512, 2171, 2187, 1288, 512, 1568, 2095, 2163, 512, 1281, 1858, 1923, 512, 1543,
	2450, 2749, 512, 2181, 1859, 2160, 512, 2183, 1918, 1988, 512, 1803, 2161, 2778, 512, 2403,
	1798, 2791, 512, 1804, 2775, 1288, 512, 2355, 2156, 2362, 512, 1281, 1640, 2380, 512, 1664,
	1731, 2052, 512, 2170, 1791, 1808, 512, 1840, 2805, 1921, 512, 2402, 2741, 1549, 512, 2046,
	1893, 2101, 512, 2159, 1801, 1288, 512, 1568, 2247, 2769, 512, 1281, 2739, 2780, 512, 1543,
	1909, 2737, 512, 2097, 2445, 2596, 512, 2757, 1794, 2024, 512, 2444, 1922, 2599, 512, 2112,
	1857, 2804, 512, 2744, 2079, 1288, 512, 2707, 2143, 1908, 512, 1281, 1640, 2782, 512, 1664,
	1731, 2169, 512, 2742, 1791, 1919, 512, 2185, 1917, 1802, 512, 2718, 1795, 1549, 512, 2098,
	2322, 2504, 512, 2581, 2350, 1288, 512, 1568, 2323, 2597, 512, 1281, 1858, 1923, 512, 1543
};

static const u16 HuffmanEncoderTable[256] =
{
	34, 437, 1159, 1735, 2584, 280, 263, 1014, 341, 839, 1687, 183, 311, 726, 920, 2761,
	599, 1417, 7945, 8073, 7642, 16186, 8890, 12858, 3913, 6362, 2746, 13882, 7866, 1080, 1273, 3400,
	886, 3386, 1097, 11482, 15450, 16282, 12506, 15578, 2377, 6858, 826, 330, 10010, 12042, 8009, 1928,
	631, 3128, 3832, 6521, 1336, 2840, 217, 5657, 121, 3865, 6553, 6426, 4666, 3017, 5193, 7994,
	3320, 1287, 1991, 71, 536, 1304, 2057, 1801, 5081, 1594, 11642, 14106, 6617, 10938, 7290, 13114,
	4809, 2522, 5818, 14010, 7482, 5914, 7738, 9018, 3450, 11450, 5897, 2697, 3193, 4185, 3769, 3464,
	3897, 968, 6841, 6393, 2425, 775, 1048, 5369, 454, 648, 3033, 3145, 2440, 2297, 200, 2872,
	2136, 2248, 1144, 1944, 1431, 1031, 376, 408, 1208, 3608, 2616, 1848, 1784, 1671, 135, 1623,
	502, 663, 1223, 2007, 248, 2104, 24, 2168, 1656, 3704, 1400, 1864, 7353, 7241, 2073, 1241,
	4889, 5690, 6153, 15738, 698, 5210, 1722, 986, 12986, 3994, 3642, 9306, 4794, 794, 16058, 7066,
	4425, 8090, 4922, 714, 11738, 7194, 12762, 7450, 5001, 1562, 11834, 13402, 9914, 3290, 3258, 5338,
	905, 15386, 9178, 15306, 3162, 15050, 15930, 10650, 15674, 8522, 8250, 7114, 10714, 14362, 9786, 2266,
	1352, 4153, 1496, 518, 151, 15482, 12410, 2952, 7961, 8906, 1114, 58, 4570, 7258, 13530, 474,
	9, 15258, 3546, 6170, 4314, 2970, 7386, 14666, 7130, 6474, 14554, 5514, 15322, 3098, 15834, 3978,
	3353, 2329, 2458, 12170, 570, 1818, 11578, 14618, 1175, 8986, 4218, 9754, 8762, 392, 8282, 11290,
	7546, 3850, 11354, 12298, 15642, 14986, 8666, 20491, 90, 13706, 12186, 6794, 11162, 10458, 759, 582
};

static UDT_FORCE_INLINE void HuffmanPutBit(u8* fout, s32 bitIndex, s32 bit)
{
	const s32 byteIndex = bitIndex >> 3;
	const s32 bitOffset = bitIndex & 7;
	if(bitOffset == 0) // Is this the first bit of a new byte?
	{
		// We don't need to preserve what's already in there,
		// so we can write that byte immediately.
		fout[byteIndex] = (u8)bit;
		return;
	}

	fout[(bitIndex >> 3)] |= bit << (bitIndex & 7);
}

static UDT_FORCE_INLINE void HuffmanOffsetTransmit(u8* fout, s32* offset, s32 ch)
{
	const u16 result = HuffmanEncoderTable[ch];
	const u16 bitCount = result & 15;
	const u16 code = (result >> 4) & 0x7FF;
	const u32 bitIndex = *(const u32*)offset;

	s32 bits = (s32)code;
	for(u32 i = 0; i < bitCount; ++i)
	{
		HuffmanPutBit(fout, bitIndex + i, bits & 1);
		bits >>= 1;
	}

	*offset += (s32)bitCount;
}


// if (s32)f == f and (s32)f + (1<<(FLOAT_INT_BITS-1)) < (1 << FLOAT_INT_BITS)
// the float value will be sent with FLOAT_INT_BITS, otherwise all 32 bits will be sent
#define	FLOAT_INT_BITS	13
#define	FLOAT_INT_BIAS	(1<<(FLOAT_INT_BITS-1))


// Offset from the start of the structure == absolute address when the struct is at address 0.
#if defined(UDT_GCC)
#	define	OFFSET_OF(type, member)		__builtin_offsetof(type, member)
#elif defined(UDT_MSVC)
#	include <stddef.h>
#	define	 OFFSET_OF(type, member)	offsetof(type, member)
#endif

//
// 3
//

#define ESF(field, bits) { (s16)OFFSET_OF(idEntityState3, field), bits }

// Each field's index is the corresponding index into the field bit mask.
const idNetField EntityStateFields3[]
{
	ESF(eType, 8),
	ESF(eFlags, 16),
	ESF(pos.trType, 8),
	ESF(pos.trTime, 32),
	ESF(pos.trDuration, 32),
	ESF(pos.trBase[0], 0),
	ESF(pos.trBase[1], 0),
	ESF(pos.trBase[2], 0),
	ESF(pos.trDelta[0], 0),
	ESF(pos.trDelta[1], 0),
	ESF(pos.trDelta[2], 0),
	ESF(apos.trType, 8),
	ESF(apos.trTime, 32),
	ESF(apos.trDuration, 32),
	ESF(apos.trBase[0], 0),
	ESF(apos.trBase[1], 0),
	ESF(apos.trBase[2], 0),
	ESF(apos.trDelta[0], 0),
	ESF(apos.trDelta[1], 0),
	ESF(apos.trDelta[2], 0),
	ESF(time, 32),
	ESF(time2, 32),
	ESF(origin[0], 0),
	ESF(origin[1], 0),
	ESF(origin[2], 0),
	ESF(origin2[0], 0),
	ESF(origin2[1], 0),
	ESF(origin2[2], 0),
	ESF(angles[0], 0),
	ESF(angles[1], 0),
	ESF(angles[2], 0),
	ESF(angles2[0], 0),
	ESF(angles2[1], 0),
	ESF(angles2[2], 0),
	ESF(otherEntityNum, 10),
	ESF(otherEntityNum2, 10),
	ESF(groundEntityNum, 10),
	ESF(loopSound, 8),
	ESF(constantLight, 32),
	ESF(modelindex, 8),
	ESF(modelindex2, 8),
	ESF(frame, 16),
	ESF(clientNum, 8),
	ESF(solid, 24),
	ESF(event, 10),
	ESF(eventParm, 8),
	ESF(powerups, 16),
	ESF(weapon, 8),
	ESF(legsAnim, 8),
	ESF(torsoAnim, 8)
};

#undef ESF

const s32 EntityStateFieldCount3 = sizeof(EntityStateFields3) / sizeof(EntityStateFields3[0]);
static_assert(EntityStateFieldCount3 == 50, "dm3 network entity states have 50 fields!");

//
// 48
//

#define ESF(field, bits) { (s16)OFFSET_OF(idEntityState48, field), bits }

// Each field's index is the corresponding index into the field bit mask.
const idNetField EntityStateFields48[]
{
	ESF(eType, 8),
	ESF(eFlags, 19), // Changed from 16 to 19...
	ESF(pos.trType, 8),
	ESF(pos.trTime, 32),
	ESF(pos.trDuration, 32),
	ESF(pos.trBase[0], 0),
	ESF(pos.trBase[1], 0),
	ESF(pos.trBase[2], 0),
	ESF(pos.trDelta[0], 0),
	ESF(pos.trDelta[1], 0),
	ESF(pos.trDelta[2], 0),
	ESF(apos.trType, 8),
	ESF(apos.trTime, 32),
	ESF(apos.trDuration, 32),
	ESF(apos.trBase[0], 0),
	ESF(apos.trBase[1], 0),
	ESF(apos.trBase[2], 0),
	ESF(apos.trDelta[0], 0),
	ESF(apos.trDelta[1], 0),
	ESF(apos.trDelta[2], 0),
	ESF(time, 32),
	ESF(time2, 32),
	ESF(origin[0], 0),
	ESF(origin[1], 0),
	ESF(origin[2], 0),
	ESF(origin2[0], 0),
	ESF(origin2[1], 0),
	ESF(origin2[2], 0),
	ESF(angles[0], 0),
	ESF(angles[1], 0),
	ESF(angles[2], 0),
	ESF(angles2[0], 0),
	ESF(angles2[1], 0),
	ESF(angles2[2], 0),
	ESF(otherEntityNum, 10),
	ESF(otherEntityNum2, 10),
	ESF(groundEntityNum, 10),
	ESF(loopSound, 8),
	ESF(constantLight, 32),
	ESF(modelindex, 8),
	ESF(modelindex2, 8),
	ESF(frame, 16),
	ESF(clientNum, 8),
	ESF(solid, 24),
	ESF(event, 10),
	ESF(eventParm, 8),
	ESF(powerups, 16),
	ESF(weapon, 8),
	ESF(legsAnim, 8),
	ESF(torsoAnim, 8),
	ESF(generic1, 8)
};

#undef ESF

const s32 EntityStateFieldCount48 = sizeof(EntityStateFields48) / sizeof(EntityStateFields48[0]);
static_assert(EntityStateFieldCount48 == 51, "dm_48 network entity states have 51 fields!");

//
// 68
//

#define ESF(field, bits) { (s16)OFFSET_OF(idEntityState68, field), bits }

const idNetField EntityStateFields68[] =
{
	ESF(pos.trTime, 32),
	ESF(pos.trBase[0], 0),
	ESF(pos.trBase[1], 0),
	ESF(pos.trDelta[0], 0),
	ESF(pos.trDelta[1], 0),
	ESF(pos.trBase[2], 0),
	ESF(apos.trBase[1], 0),
	ESF(pos.trDelta[2], 0),
	ESF(apos.trBase[0], 0),
	ESF(event, 10),
	ESF(angles2[1], 0),
	ESF(eType, 8),
	ESF(torsoAnim, 8),
	ESF(eventParm, 8),
	ESF(legsAnim, 8),
	ESF(groundEntityNum, GENTITYNUM_BITS),
	ESF(pos.trType, 8),
	ESF(eFlags, 19),
	ESF(otherEntityNum, GENTITYNUM_BITS),
	ESF(weapon, 8),
	ESF(clientNum, 8),
	ESF(angles[1], 0),
	ESF(pos.trDuration, 32),
	ESF(apos.trType, 8),
	ESF(origin[0], 0),
	ESF(origin[1], 0),
	ESF(origin[2], 0),
	ESF(solid, 24),
	ESF(powerups, ID_MAX_PS_POWERUPS),
	ESF(modelindex, 8),
	ESF(otherEntityNum2, GENTITYNUM_BITS),
	ESF(loopSound, 8),
	ESF(generic1, 8),
	ESF(origin2[2], 0),
	ESF(origin2[0], 0),
	ESF(origin2[1], 0),
	ESF(modelindex2, 8),
	ESF(angles[0], 0),
	ESF(time, 32),
	ESF(apos.trTime, 32),
	ESF(apos.trDuration, 32),
	ESF(apos.trBase[2], 0),
	ESF(apos.trDelta[0], 0),
	ESF(apos.trDelta[1], 0),
	ESF(apos.trDelta[2], 0),
	ESF(time2, 32),
	ESF(angles[2], 0),
	ESF(angles2[0], 0),
	ESF(angles2[2], 0),
	ESF(constantLight, 32),
	ESF(frame, 16)
};

#undef ESF

const s32 EntityStateFieldCount68 = sizeof(EntityStateFields68) / sizeof(EntityStateFields68[0]);

//
// 73
//

#define ESF(field, bits) { (s16)OFFSET_OF(idEntityState73, field), bits }

const idNetField EntityStateFields73[] =
{
	ESF(pos.trTime, 32),
	ESF(pos.trBase[0], 0),
	ESF(pos.trBase[1], 0),
	ESF(pos.trDelta[0], 0),
	ESF(pos.trDelta[1], 0),
	ESF(pos.trBase[2], 0),
	ESF(apos.trBase[1], 0),
	ESF(pos.trDelta[2], 0),
	ESF(apos.trBase[0], 0),
	ESF(pos_gravity, 32), // New in dm_73
	ESF(event, 10),
	ESF(angles2[1], 0),
	ESF(eType, 8),
	ESF(torsoAnim, 8),
	ESF(eventParm, 8),
	ESF(legsAnim, 8),
	ESF(groundEntityNum, GENTITYNUM_BITS),
	ESF(pos.trType, 8),
	ESF(eFlags, 19),
	ESF(otherEntityNum, GENTITYNUM_BITS),
	ESF(weapon, 8),
	ESF(clientNum, 8),
	ESF(angles[1], 0),
	ESF(pos.trDuration, 32),
	ESF(apos.trType, 8),
	ESF(origin[0], 0),
	ESF(origin[1], 0),
	ESF(origin[2], 0),
	ESF(solid, 24),
	ESF(powerups, 16),
	ESF(modelindex, 8),
	ESF(otherEntityNum2, GENTITYNUM_BITS),
	ESF(loopSound, 8),
	ESF(generic1, 8),
	ESF(origin2[2], 0),
	ESF(origin2[0], 0),
	ESF(origin2[1], 0),
	ESF(modelindex2, 8),
	ESF(angles[0], 0),
	ESF(time, 32),
	ESF(apos.trTime, 32),
	ESF(apos.trDuration, 32),
	ESF(apos.trBase[2], 0),
	ESF(apos.trDelta[0], 0),
	ESF(apos.trDelta[1], 0),
	ESF(apos.trDelta[2], 0),
	ESF(apos_gravity, 32), // New in dm_73
	ESF(time2, 32),
	ESF(angles[2], 0),
	ESF(angles2[0], 0),
	ESF(angles2[2], 0),
	ESF(constantLight, 32),
	ESF(frame, 16)
};

#undef ESF

const s32 EntityStateFieldCount73 = sizeof(EntityStateFields73) / sizeof(EntityStateFields73[0]);

//
// 90
//

#define ESF(field, bits) { (s16)OFFSET_OF(idEntityState90, field), bits }

const idNetField EntityStateFields90[] =
{
	ESF(pos.trTime, 32),
	ESF(pos.trBase[0], 0),
	ESF(pos.trBase[1], 0),
	ESF(pos.trDelta[0], 0),
	ESF(pos.trDelta[1], 0),
	ESF(pos.trBase[2], 0),
	ESF(apos.trBase[1], 0),
	ESF(pos.trDelta[2], 0),
	ESF(apos.trBase[0], 0),
	ESF(pos_gravity, 32), // New in dm_73
	ESF(event, 10),
	ESF(angles2[1], 0),
	ESF(eType, 8),
	ESF(torsoAnim, 8),
	ESF(eventParm, 8),
	ESF(legsAnim, 8),
	ESF(groundEntityNum, GENTITYNUM_BITS),
	ESF(pos.trType, 8),
	ESF(eFlags, 19),
	ESF(otherEntityNum, GENTITYNUM_BITS),
	ESF(weapon, 8),
	ESF(clientNum, 8),
	ESF(angles[1], 0),
	ESF(pos.trDuration, 32),
	ESF(apos.trType, 8),
	ESF(origin[0], 0),
	ESF(origin[1], 0),
	ESF(origin[2], 0),
	ESF(solid, 24),
	ESF(powerups, 16),
	ESF(modelindex, 8),
	ESF(otherEntityNum2, GENTITYNUM_BITS),
	ESF(loopSound, 8),
	ESF(generic1, 8),
	ESF(origin2[2], 0),
	ESF(origin2[0], 0),
	ESF(origin2[1], 0),
	ESF(modelindex2, 8),
	ESF(angles[0], 0),
	ESF(time, 32),
	ESF(apos.trTime, 32),
	ESF(apos.trDuration, 32),
	ESF(apos.trBase[2], 0),
	ESF(apos.trDelta[0], 0),
	ESF(apos.trDelta[1], 0),
	ESF(apos.trDelta[2], 0),
	ESF(apos_gravity, 32), // New in dm_73
	ESF(time2, 32),
	ESF(angles[2], 0),
	ESF(angles2[0], 0),
	ESF(angles2[2], 0),
	ESF(constantLight, 32),
	ESF(frame, 16),
	ESF(jumpTime, 32),   // New in dm_90
	ESF(doubleJumped, 1) // New in dm_90
};

#undef ESF

const s32 EntityStateFieldCount90 = sizeof(EntityStateFields90) / sizeof(EntityStateFields90[0]);

//
// 91
//

#define ESF(field, bits) { (s16)OFFSET_OF(idEntityState91, field), bits }

const idNetField EntityStateFields91[] =
{
	ESF(pos.trTime, 32),
	ESF(pos.trBase[0], 0),
	ESF(pos.trBase[1], 0),
	ESF(pos.trDelta[0], 0),
	ESF(pos.trDelta[1], 0),
	ESF(pos.trBase[2], 0),
	ESF(apos.trBase[1], 0),
	ESF(pos.trDelta[2], 0),
	ESF(apos.trBase[0], 0),
	ESF(pos_gravity, 32),
	ESF(event, 10),
	ESF(angles2[1], 0),
	ESF(eType, 8),
	ESF(torsoAnim, 8),
	ESF(eventParm, 8),
	ESF(legsAnim, 8),
	ESF(groundEntityNum, GENTITYNUM_BITS),
	ESF(pos.trType, 8),
	ESF(eFlags, 19),
	ESF(otherEntityNum, GENTITYNUM_BITS),
	ESF(weapon, 8),
	ESF(clientNum, 8),
	ESF(angles[1], 0),
	ESF(pos.trDuration, 32),
	ESF(apos.trType, 8),
	ESF(origin[0], 0),
	ESF(origin[1], 0),
	ESF(origin[2], 0),
	ESF(solid, 24),
	ESF(powerups, 16),
	ESF(modelindex, 8),
	ESF(otherEntityNum2, GENTITYNUM_BITS),
	ESF(loopSound, 8),
	ESF(generic1, 8),
	ESF(origin2[2], 0),
	ESF(origin2[0], 0),
	ESF(origin2[1], 0),
	ESF(modelindex2, 8),
	ESF(angles[0], 0),
	ESF(time, 32),
	ESF(apos.trTime, 32),
	ESF(apos.trDuration, 32),
	ESF(apos.trBase[2], 0),
	ESF(apos.trDelta[0], 0),
	ESF(apos.trDelta[1], 0),
	ESF(apos.trDelta[2], 0),
	ESF(apos_gravity, 32),
	ESF(time2, 32),
	ESF(angles[2], 0),
	ESF(angles2[0], 0),
	ESF(angles2[2], 0),
	ESF(constantLight, 32),
	ESF(frame, 16),
	ESF(jumpTime, 32),
	ESF(doubleJumped, 1),
	ESF(health, 16), // New in protocol 91.
	ESF(armor, 16),  // New in protocol 91.
	ESF(location, 8) // New in protocol 91.
};

#undef ESF

const s32 EntityStateFieldCount91 = sizeof(EntityStateFields91) / sizeof(EntityStateFields91[0]);

//
// 3
//

#define PSF(field, bits) { (s16)OFFSET_OF(idPlayerState3, field), bits }

static const idNetField PlayerStateFields3[] =
{
	PSF(commandTime, 32),
	PSF(pm_type, 8),
	PSF(bobCycle, 8),
	PSF(pm_flags, 16),
	PSF(pm_time, 16),
	PSF(origin[0], 0),
	PSF(origin[1], 0),
	PSF(origin[2], 0),
	PSF(velocity[0], 0),
	PSF(velocity[1], 0),
	PSF(velocity[2], 0),
	PSF(weaponTime, 16),
	PSF(gravity, 16),
	PSF(speed, 16),
	PSF(delta_angles[0], 16),
	PSF(delta_angles[1], 16),
	PSF(delta_angles[2], 16),
	PSF(groundEntityNum, 10),
	PSF(legsTimer, 8),
	PSF(torsoTimer, 12),
	PSF(legsAnim, 8),
	PSF(torsoAnim, 8),
	PSF(movementDir, 4),
	PSF(eFlags, 16),
	PSF(eventSequence, 16),
	PSF(events[0], 8),
	PSF(events[1], 8),
	PSF(eventParms[0], 8),
	PSF(eventParms[1], 8),
	PSF(externalEvent, 8),
	PSF(externalEventParm, 8),
	PSF(clientNum, 8),
	PSF(weapon, 5),
	PSF(weaponstate, 4),
	PSF(viewangles[0], 0),
	PSF(viewangles[1], 0),
	PSF(viewangles[2], 0),
	PSF(viewheight, 8),
	PSF(damageEvent, 8),
	PSF(damageYaw, 8),
	PSF(damagePitch, 8),
	PSF(damageCount, 8),
	PSF(grapplePoint[0], 0),
	PSF(grapplePoint[1], 0),
	PSF(grapplePoint[2], 0)
};

#undef PSF

static const s32 PlayerStateFieldCount3 = sizeof(PlayerStateFields3) / sizeof(PlayerStateFields3[0]);

//
// 48
//

#define PSF(field, bits) { (s16)OFFSET_OF(idPlayerState48, field), bits }

static const idNetField PlayerStateFields48[] =
{
	PSF(commandTime, 32),
	PSF(pm_type, 8),
	PSF(bobCycle, 8),
	PSF(pm_flags, 16),
	PSF(pm_time, -16),
	PSF(origin[0], 0),
	PSF(origin[1], 0),
	PSF(origin[2], 0),
	PSF(velocity[0], 0),
	PSF(velocity[1], 0),
	PSF(velocity[2], 0),
	PSF(weaponTime, -16),
	PSF(gravity, 16),
	PSF(speed, 16),
	PSF(delta_angles[0], 16),
	PSF(delta_angles[1], 16),
	PSF(delta_angles[2], 16),
	PSF(groundEntityNum, 10),
	PSF(legsTimer, 8),
	PSF(torsoTimer, 12),
	PSF(legsAnim, 8),
	PSF(torsoAnim, 8),
	PSF(movementDir, 4),
	PSF(eFlags, 16),
	PSF(eventSequence, 16),
	PSF(events[0], 8),
	PSF(events[1], 8),
	PSF(eventParms[0], 8),
	PSF(eventParms[1], 8),
	PSF(externalEvent, 10), // Changed from 8 to 10...
	PSF(externalEventParm, 8),
	PSF(clientNum, 8),
	PSF(weapon, 5),
	PSF(weaponstate, 4),
	PSF(viewangles[0], 0),
	PSF(viewangles[1], 0),
	PSF(viewangles[2], 0),
	PSF(viewheight, -8),
	PSF(damageEvent, 8),
	PSF(damageYaw, 8),
	PSF(damagePitch, 8),
	PSF(damageCount, 8), 
	PSF(grapplePoint[0], 0),
	PSF(grapplePoint[1], 0),
	PSF(grapplePoint[2], 0),
	PSF(jumppad_ent, 10), // New in dm_48.
	PSF(loopSound, 16),   // New in dm_48.
	PSF(generic1, 8)      // New in dm_48.
};

#undef PSF

static const s32 PlayerStateFieldCount48 = sizeof(PlayerStateFields48) / sizeof(PlayerStateFields48[0]);

//
// 68
//

#define PSF(field, bits) { (s16)OFFSET_OF(idPlayerState68, field), bits }

static const idNetField PlayerStateFields68[] =
{
	PSF(commandTime, 32),
	PSF(origin[0], 0),
	PSF(origin[1], 0),
	PSF(bobCycle, 8),
	PSF(velocity[0], 0),
	PSF(velocity[1], 0),
	PSF(viewangles[1], 0),
	PSF(viewangles[0], 0),
	PSF(weaponTime, -16),
	PSF(origin[2], 0),
	PSF(velocity[2], 0),
	PSF(legsTimer, 8),
	PSF(pm_time, -16),
	PSF(eventSequence, 16),
	PSF(torsoAnim, 8),
	PSF(movementDir, 4),
	PSF(events[0], 8),
	PSF(legsAnim, 8),
	PSF(events[1], 8),
	PSF(pm_flags, 16),
	PSF(groundEntityNum, GENTITYNUM_BITS),
	PSF(weaponstate, 4),
	PSF(eFlags, 16),
	PSF(externalEvent, 10),
	PSF(gravity, 16),
	PSF(speed, 16),
	PSF(delta_angles[1], 16),
	PSF(externalEventParm, 8),
	PSF(viewheight, -8),
	PSF(damageEvent, 8),
	PSF(damageYaw, 8),
	PSF(damagePitch, 8),
	PSF(damageCount, 8),
	PSF(generic1, 8),
	PSF(pm_type, 8),
	PSF(delta_angles[0], 16),
	PSF(delta_angles[2], 16),
	PSF(torsoTimer, 12),
	PSF(eventParms[0], 8),
	PSF(eventParms[1], 8),
	PSF(clientNum, 8),
	PSF(weapon, 5),
	PSF(viewangles[2], 0),
	PSF(grapplePoint[0], 0),
	PSF(grapplePoint[1], 0),
	PSF(grapplePoint[2], 0),
	PSF(jumppad_ent, 10),
	PSF(loopSound, 16)
};

#undef PSF

static const s32 PlayerStateFieldCount68 = sizeof(PlayerStateFields68) / sizeof(PlayerStateFields68[0]);

//
// 73
//

#define PSF(field, bits) { (s16)OFFSET_OF(idPlayerState73, field), bits }

static const idNetField PlayerStateFields73[] =
{
	PSF(commandTime, 32),
	PSF(origin[0], 0),
	PSF(origin[1], 0),
	PSF(bobCycle, 8),
	PSF(velocity[0], 0),
	PSF(velocity[1], 0),
	PSF(viewangles[1], 0),
	PSF(viewangles[0], 0),
	PSF(weaponTime, -16),
	PSF(origin[2], 0),
	PSF(velocity[2], 0),
	PSF(legsTimer, 8),
	PSF(pm_time, -16),
	PSF(eventSequence, 16),
	PSF(torsoAnim, 8),
	PSF(movementDir, 4),
	PSF(events[0], 8),
	PSF(legsAnim, 8),
	PSF(events[1], 8),
	PSF(pm_flags, 16),
	PSF(groundEntityNum, GENTITYNUM_BITS),
	PSF(weaponstate, 4),
	PSF(eFlags, 16),
	PSF(externalEvent, 10),
	PSF(gravity, 16),
	PSF(speed, 16),
	PSF(delta_angles[1], 16),
	PSF(externalEventParm, 8),
	PSF(viewheight, -8),
	PSF(damageEvent, 8),
	PSF(damageYaw, 8),
	PSF(damagePitch, 8),
	PSF(damageCount, 8),
	PSF(generic1, 8),
	PSF(pm_type, 8),
	PSF(delta_angles[0], 16),
	PSF(delta_angles[2], 16),
	PSF(torsoTimer, 12),
	PSF(eventParms[0], 8),
	PSF(eventParms[1], 8),
	PSF(clientNum, 8),
	PSF(weapon, 5),
	PSF(viewangles[2], 0),
	PSF(grapplePoint[0], 0),
	PSF(grapplePoint[1], 0),
	PSF(grapplePoint[2], 0),
	PSF(jumppad_ent, 10),
	PSF(loopSound, 16)
};

#undef PSF

static const s32 PlayerStateFieldCount73 = sizeof(PlayerStateFields73) / sizeof(PlayerStateFields73[0]);

//
// 90
//

#define PSF(field, bits) { (s16)OFFSET_OF(idPlayerState90, field), bits }

static const idNetField PlayerStateFields90[] =
{
	PSF(commandTime, 32),
	PSF(origin[0], 0),
	PSF(origin[1], 0),
	PSF(bobCycle, 8),
	PSF(velocity[0], 0),
	PSF(velocity[1], 0),
	PSF(viewangles[1], 0),
	PSF(viewangles[0], 0),
	PSF(weaponTime, -16),
	PSF(origin[2], 0),
	PSF(velocity[2], 0),
	PSF(legsTimer, 8),
	PSF(pm_time, -16),
	PSF(eventSequence, 16),
	PSF(torsoAnim, 8),
	PSF(movementDir, 4),
	PSF(events[0], 8),
	PSF(legsAnim, 8),
	PSF(events[1], 8),
	PSF(pm_flags, 24),
	PSF(groundEntityNum, GENTITYNUM_BITS),
	PSF(weaponstate, 4),
	PSF(eFlags, 16),
	PSF(externalEvent, 10),
	PSF(gravity, 16),
	PSF(speed, 16),
	PSF(delta_angles[1], 16),
	PSF(externalEventParm, 8),
	PSF(viewheight, -8),
	PSF(damageEvent, 8),
	PSF(damageYaw, 8),
	PSF(damagePitch, 8),
	PSF(damageCount, 8),
	PSF(generic1, 8),
	PSF(pm_type, 8),
	PSF(delta_angles[0], 16),
	PSF(delta_angles[2], 16),
	PSF(torsoTimer, 12),
	PSF(eventParms[0], 8),
	PSF(eventParms[1], 8),
	PSF(clientNum, 8),
	PSF(weapon, 5),
	PSF(viewangles[2], 0),
	PSF(grapplePoint[0], 0), // New in protocol 90.
	PSF(grapplePoint[1], 0), // New in protocol 90.
	PSF(grapplePoint[2], 0), // New in protocol 90.
	PSF(jumppad_ent, 10),
	PSF(loopSound, 16),
	PSF(jumpTime, 32),   // New in protocol 90.
	PSF(doubleJumped, 1) // New in protocol 90.
};

#undef PSF

static const s32 PlayerStateFieldCount90 = sizeof(PlayerStateFields90) / sizeof(PlayerStateFields90[0]);

//
// 91
//

#define PSF(field, bits) { (s16)OFFSET_OF(idPlayerState91, field), bits }

static const idNetField PlayerStateFields91[] =
{
	PSF(commandTime, 32),
	PSF(origin[0], 0),
	PSF(origin[1], 0),
	PSF(bobCycle, 8),
	PSF(velocity[0], 0),
	PSF(velocity[1], 0),
	PSF(viewangles[1], 0),
	PSF(viewangles[0], 0),
	PSF(weaponTime, -16),
	PSF(origin[2], 0),
	PSF(velocity[2], 0),
	PSF(legsTimer, 8),
	PSF(pm_time, -16),
	PSF(eventSequence, 16),
	PSF(torsoAnim, 8),
	PSF(movementDir, 4),
	PSF(events[0], 8),
	PSF(legsAnim, 8),
	PSF(events[1], 8),
	PSF(pm_flags, 24),
	PSF(groundEntityNum, GENTITYNUM_BITS),
	PSF(weaponstate, 4),
	PSF(eFlags, 16),
	PSF(externalEvent, 10),
	PSF(gravity, 16),
	PSF(speed, 16),
	PSF(delta_angles[1], 16),
	PSF(externalEventParm, 8),
	PSF(viewheight, -8),
	PSF(damageEvent, 8),
	PSF(damageYaw, 8),
	PSF(damagePitch, 8),
	PSF(damageCount, 8),
	PSF(generic1, 8),
	PSF(pm_type, 8),
	PSF(delta_angles[0], 16),
	PSF(delta_angles[2], 16),
	PSF(torsoTimer, 12),
	PSF(eventParms[0], 8),
	PSF(eventParms[1], 8),
	PSF(clientNum, 8),
	PSF(weapon, 5),
	PSF(weaponPrimary, 8),
	PSF(viewangles[2], 0),
	PSF(grapplePoint[0], 0),
	PSF(grapplePoint[1], 0),
	PSF(grapplePoint[2], 0),
	PSF(jumppad_ent, 10),
	PSF(loopSound, 16),
	PSF(jumpTime, 32),
	PSF(doubleJumped, 1),
	PSF(crouchTime, 32),      // New in protocol 91.
	PSF(crouchSlideTime, 32), // New in protocol 91.
	PSF(location, 8),         // New in protocol 91.
	PSF(fov, 8),              // New in protocol 91.
	PSF(forwardmove, 8),      // New in protocol 91.
	PSF(rightmove, 8),        // New in protocol 91.
	PSF(upmove, 8)            // New in protocol 91.
};

#undef PSF

static const s32 PlayerStateFieldCount91 = sizeof(PlayerStateFields91) / sizeof(PlayerStateFields91[0]);


udtMessage::udtMessage()
{
	_protocol = udtProtocol::Dm68;
	_protocolSizeOfEntityState = sizeof(idEntityState68);
	_protocolSizeOfPlayerState = sizeof(idPlayerState68);
	_entityStateFields = EntityStateFields68;
	_entityStateFieldCount = EntityStateFieldCount68;
	_playerStateFields = PlayerStateFields68;
	_playerStateFieldCount = PlayerStateFieldCount68;
	_fileName = udtString::NewNull();
}

void udtMessage::InitContext(udtContext* context)
{
	Context = context; 
}

void udtMessage::InitProtocol(udtProtocol::Id protocol)
{
	_protocol = protocol;

	switch(protocol)
	{
		case udtProtocol::Dm91:
			_protocolSizeOfEntityState = sizeof(idEntityState91);
			_protocolSizeOfPlayerState = sizeof(idPlayerState91);
			_entityStateFields = EntityStateFields91;
			_entityStateFieldCount = EntityStateFieldCount91;
			_playerStateFields = PlayerStateFields91;
			_playerStateFieldCount = PlayerStateFieldCount91;
			break;

		case udtProtocol::Dm90:
			_protocolSizeOfEntityState = sizeof(idEntityState90);
			_protocolSizeOfPlayerState = sizeof(idPlayerState90);
			_entityStateFields = EntityStateFields90;
			_entityStateFieldCount = EntityStateFieldCount90;
			_playerStateFields = PlayerStateFields90;
			_playerStateFieldCount = PlayerStateFieldCount90;
			break;

		case udtProtocol::Dm73:
			_protocolSizeOfEntityState = sizeof(idEntityState73);
			_protocolSizeOfPlayerState = sizeof(idPlayerState73);
			_entityStateFields = EntityStateFields73;
			_entityStateFieldCount = EntityStateFieldCount73;
			_playerStateFields = PlayerStateFields73;
			_playerStateFieldCount = PlayerStateFieldCount73;
			break;

		case udtProtocol::Dm3:
			_protocolSizeOfEntityState = sizeof(idEntityState3);
			_protocolSizeOfPlayerState = sizeof(idPlayerState3);
			_entityStateFields = EntityStateFields3;
			_entityStateFieldCount = EntityStateFieldCount3;
			_playerStateFields = PlayerStateFields3;
			_playerStateFieldCount = PlayerStateFieldCount3;
			break;

		case udtProtocol::Dm48:
			_protocolSizeOfEntityState = sizeof(idEntityState48);
			_protocolSizeOfPlayerState = sizeof(idPlayerState48);
			_entityStateFields = EntityStateFields48;
			_entityStateFieldCount = EntityStateFieldCount48;
			_playerStateFields = PlayerStateFields48;
			_playerStateFieldCount = PlayerStateFieldCount48;
			break;

		case udtProtocol::Dm66:
			_protocolSizeOfEntityState = sizeof(idEntityState66);
			_protocolSizeOfPlayerState = sizeof(idPlayerState66);
			_entityStateFields = EntityStateFields68;
			_entityStateFieldCount = EntityStateFieldCount68;
			_playerStateFields = PlayerStateFields68;
			_playerStateFieldCount = PlayerStateFieldCount68;
			break;

		case udtProtocol::Dm67:
			_protocolSizeOfEntityState = sizeof(idEntityState67);
			_protocolSizeOfPlayerState = sizeof(idPlayerState67);
			_entityStateFields = EntityStateFields68;
			_entityStateFieldCount = EntityStateFieldCount68;
			_playerStateFields = PlayerStateFields68;
			_playerStateFieldCount = PlayerStateFieldCount68;
			break;

		case udtProtocol::Dm68:
		default:
			_protocolSizeOfEntityState = sizeof(idEntityState68);
			_protocolSizeOfPlayerState = sizeof(idPlayerState68);
			_entityStateFields = EntityStateFields68;
			_entityStateFieldCount = EntityStateFieldCount68;
			_playerStateFields = PlayerStateFields68;
			_playerStateFieldCount = PlayerStateFieldCount68;
			break;
	}
}

void udtMessage::Init(u8* data, s32 length) 
{
	Com_Memset(&Buffer, 0, sizeof(idMessage));
	Buffer.data = data;
	Buffer.maxsize = length;
	SetValid(true);
}

void udtMessage::Bitstream() 
{
	Buffer.oob = false;
	SetValid(Buffer.valid);
}

void udtMessage::SetHuffman(bool huffman)
{
	Buffer.oob = huffman ? false : true;
	SetValid(Buffer.valid);
}

void udtMessage::GoToNextByte()
{
	if((Buffer.bit & 7) != 0)
	{
		++Buffer.readcount;
		Buffer.bit = Buffer.readcount << 3;
	}
}

// negative bit values include signs
void udtMessage::RealWriteBits(s32 value, s32 signedBits)
{
	const bool signedValue = signedBits < 0;
	s32 bits = signedValue ? -signedBits : signedBits;
	if(Buffer.bit + bits > Buffer.maxsize * 8)
	{
		Context->LogError("udtMessage::RealWriteBits: Overflowed! (in file: %s)", GetFileNamePtr());
		SetValid(false);
		return;
	}

	if(signedBits == 0 || signedBits < -31 || signedBits > 32)
	{
		Context->LogError("udtMessage::RealWriteBits: Invalid bit count: %d (in file: %s)", bits, GetFileNamePtr());
		SetValid(false);
		return;
	}

	if(Buffer.oob) 
	{
		if(bits == 8)
		{
			Buffer.data[Buffer.cursize] = (u8)value;
			Buffer.cursize += 1;
			Buffer.bit += 8;
		} 
		else if(bits == 16) 
		{
			unsigned short* sp = (unsigned short*)&Buffer.data[Buffer.cursize];
			*sp = (unsigned short)value;
			Buffer.cursize += 2;
			Buffer.bit += 16;
		} 
		else if(bits == 32) 
		{
			u32* ip = (u32*)&Buffer.data[Buffer.cursize];
			*ip = value;
			Buffer.cursize += 4;
			Buffer.bit += 32;
		} 
		else 
		{
			Context->LogError("udtMessage::RealWriteBits: Can't write %d bits (in file: %s)", bits, GetFileNamePtr());
			SetValid(false);
			return;
		}
	} 
	else 
	{
		value &= (0xffffffff >> (32-bits));

		if(bits & 7) 
		{
			s32 nbits = bits&7;
			for(s32 i = 0; i < nbits; ++i) 
			{
				HuffmanPutBit(Buffer.data, Buffer.bit, value & 1);
				value = value >> 1;
				++Buffer.bit;
			}
			bits = bits - nbits;
		}

		if(bits) 
		{
			for(s32 i = 0; i < bits; i += 8) 
			{
				HuffmanOffsetTransmit(Buffer.data, &Buffer.bit, value & 0xFF);
				value = value >> 8;
			}
		}

		Buffer.cursize = (Buffer.bit >> 3) + 1;
	}
}

s32 udtMessage::RealReadBits(s32 signedBits)
{
	//
	// Allow up to 4 bytes of overflow. This is needed because otherwise some demos 
	// that Quake can parse properly don't get fully parsed by UDT.
	//
	// In the Huffman case, the number of bits that will actually be read is not known in advance.
	// Instead of checking for an overflow after every symbol decode, we check once before using
	// an upper bound. The maximum code length is 11 bits.
	// > full_bytes = bits / 8
	// > remainder_bits = bits % 8
	// > max_bits_read = full_bytes*11 + remainder_bits
	// The maximum request length is 32 bits or 4 symbols. That is, the maximum number of bits read 
	// would be 4 x 11 = 44 bits and thus the maximum overflow 44 - 32 = 12 bits.
	//
	const bool signedValue = signedBits < 0;
	s32 bits = signedValue ? -signedBits : signedBits;
	if(Buffer.bit + bits > (Buffer.cursize + 4) * 8)
	{
		Context->LogError("udtMessage::RealReadBits: Overflowed! (in file: %s)", GetFileNamePtr());
		SetValid(false);
		return -1;
	}

	const u8* const bufferData = Buffer.data;
	s32 value = 0;
	if(Buffer.oob) 
	{
		if(_protocol >= udtProtocol::Dm68)
		{
			if(bits == 8)
			{
				value = bufferData[Buffer.readcount];
				Buffer.readcount += 1;
				Buffer.bit += 8;
			}
			else if(bits == 16)
			{
				value = s32(*(const u16*)&bufferData[Buffer.readcount]);
				Buffer.readcount += 2;
				Buffer.bit += 16;
			}
			else if(bits == 32)
			{
				value = s32(*(const u32*)&bufferData[Buffer.readcount]);
				Buffer.readcount += 4;
				Buffer.bit += 32;
			}
			else
			{
				Context->LogError("udtMessage::RealReadBits: Can't read %d bits (in file: %s)", bits, GetFileNamePtr());
				SetValid(false);
				return -1;
			}
		}
		else
		{
			if(bits > 32)
			{
				Context->LogError("udtMessage::RealReadBits: Can't read %d bits (more than 32) (in file: %s)", bits, GetFileNamePtr());
				SetValid(false);
				return -1;
			}

			u64 readBits = *(const u64*)&bufferData[Buffer.readcount];
			const u64 bitPosition = (u64)Buffer.bit & 7;
			const u64 diff = 64 - (u64)bits;
			readBits >>= bitPosition;
			readBits <<= diff;
			readBits >>= diff;
			value = (s32)readBits;
			Buffer.bit += bits;
			Buffer.readcount = Buffer.bit >> 3;
		}
	} 
	else
	{
		const s32 nbits = bits & 7;
		s32 bitIndex = Buffer.bit;
		if(nbits)
		{		
			const s16 allBits = *(const s16*)(bufferData + (bitIndex >> 3)) >> (bitIndex & 7);
			value = allBits & ((1 << nbits) - 1);
			bitIndex += nbits;
			bits -= nbits;
		}

		if(bits)
		{
			for(s32 i = 0; i < bits; i += 8)
			{
				const u16 code = ((*(const u32*)(bufferData + (bitIndex >> 3))) >> ((u32)bitIndex & 7)) & 0x7FF;
				const u16 entry = HuffmanDecoderTable[code];
				value |= (s32(entry & 0xFF) << (i + nbits));
				bitIndex += s32(entry >> 8);
			}
		}

		Buffer.bit = bitIndex;
		Buffer.readcount = (bitIndex >> 3) + 1;
	}

	// If signed, we need to replicate the bit sign.
	if(signedValue)
	{
		const s32 bitCount = 32 - bits;

		// Yes, right shifting a negative signed number has implementation-defined behavior
		// but we're fine on that front with Visual Studio and GCC on x86 and x64.
		return (value << bitCount) >> bitCount;
	}

	return value;
}

s32 udtMessage::RealReadBitNoHuffman()
{
	// @NOTE: We leave overflow checking to RealReadBits.
	// There is no way we call this enough times in a row to get an overflow.
	const u8 byte = Buffer.data[Buffer.readcount] >> ((u8)Buffer.bit & 7);
	const s32 value = s32(byte & 1);
	const s32 newBitCount = Buffer.bit + 1;
	Buffer.bit = newBitCount;
	Buffer.readcount = newBitCount >> 3;

	return value;
}

s32 udtMessage::RealReadBitHuffman()
{
	// @NOTE: We leave overflow checking to RealReadBits.
	// There is no way we call this enough times in a row to get an overflow.
	const s32 bitCount = Buffer.bit;
	const u8 byte = Buffer.data[bitCount >> 3] >> (u8)(bitCount & 7);
	const s32 value = s32(byte & 1);
	const s32 newBitCount = bitCount + 1;
	Buffer.bit = newBitCount;
	Buffer.readcount = (newBitCount >> 3) + 1;

	return value;
}

void udtMessage::WriteData(const void* data, s32 length) 
{
	for(s32 i = 0; i < length; ++i) 
	{
		WriteByte(((u8*)data)[i]);
	}
}

void udtMessage::RealWriteFloat(s32 c)
{
	// Sneaking around the strict aliasing rules.
	union IntAndFloat
	{
		IntAndFloat(s32 i) : AsInt(i) {}

		s32 AsInt;
		f32 AsFloat;
	};

	const IntAndFloat fullFloatUnion(c);
	const f32 fullFloat = fullFloatUnion.AsFloat;
	const s32 truncatedFloat = (s32)fullFloat;
	if(truncatedFloat == fullFloat && 
	   truncatedFloat + FLOAT_INT_BIAS >= 0 &&
	   truncatedFloat + FLOAT_INT_BIAS < (1 << FLOAT_INT_BITS))
	{
		// Small integer case.
		WriteBits(0, 1);
		WriteBits(truncatedFloat + FLOAT_INT_BIAS, FLOAT_INT_BITS);
	}
	else
	{
		// Full floating-point value.
		WriteBits(1, 1);
		WriteBits(c, 32);
	}
}

void udtMessage::RealWriteString(const char* s, s32 length, s32 bufferLength, char* buffer)
{
	if(!s) 
	{
		WriteData("", 1);
		return;
	} 
	
	if(length >= bufferLength)
	{
		Context->LogError("udtMessage::RealWriteString: The string is too long: %d (max is %d) (in file: %s)", length, bufferLength, GetFileNamePtr());
		SetValid(false);
		return;
	}

	if(_protocol >= udtProtocol::Dm91)
	{
		WriteData(s, length + 1);
		return;
	}

	Q_strncpyz(buffer, s, bufferLength);

	// get rid of 0xff bytes, because old clients don't like them
	for(s32 i = 0; i < length; ++i)
	{
		if(((u8*)buffer)[i] > 127)
		{
			buffer[i] = '.';
		}
	}

	WriteData(buffer, length + 1);
}

s32 udtMessage::RealReadFloat()
{
	if(ReadBit())
	{
		return ReadBits(32);
	}

	// Sneaking around the strict aliasing rules.
	union FloatAndInt
	{
		FloatAndInt(f32 f) : AsFloat(f) {}

		f32 AsFloat;
		s32 AsInt;
	};

	const s32 intValue = ReadBits(FLOAT_INT_BITS) - FLOAT_INT_BIAS;
	const FloatAndInt realValue((f32)intValue);
	
	return realValue.AsInt;
}

char* udtMessage::RealReadString(s32& length, s32 bufferLength, char* buffer)
{
	s32 stringLength = 0;
	do
	{
		// use ReadByte so -1 is out of bounds
		s32 c = ReadByte();
		if(c == -1 || c == 0)
		{
			break;
		}

		// translate all fmt spec to avoid crash bugs
		if(c == '%')
		{
			c = '.';
		}

		// don't allow higher ascii values
		if(_protocol <= udtProtocol::Dm90 && c > 127)
		{
			c = '.';
		}

		buffer[stringLength] = (s8)c;
		stringLength++;
	}
	while(stringLength < bufferLength - 1);

	buffer[stringLength] = 0;
	length = stringLength;

	return buffer;
}

void udtMessage::RealReadData(void* data, s32 len) 
{
	for(s32 i = 0; i < len; ++i) 
	{
		((u8*)data)[i] = (u8)ReadByte();
	}
}

s32 udtMessage::RealPeekByte()
{
	// @TODO: Allow up to 4 bytes of overflow like in RealReadBits?
	if(Buffer.bit + 8 > (Buffer.cursize + 1) * 8)
	{
		Context->LogError("udtMessage::RealPeekByte: Overflowed! (in file: %s)", GetFileNamePtr());
		SetValid(false);
		return -1;
	}

	const s32 readcount = Buffer.readcount;
	const s32 bit = Buffer.bit;
	const s32 c = ReadByte();
	Buffer.readcount = readcount;
	Buffer.bit = bit;

	return c;
}

bool udtMessage::RealWriteDeltaPlayer(const idPlayerStateBase* from, idPlayerStateBase* to)
{
	s32				i;
	idLargestPlayerState dummy;
	s32				*fromF, *toF;
	s32				lc;

	if(!from) 
	{
		from = &dummy;
		Com_Memset (&dummy, 0, sizeof(dummy));
	}

	lc = 0;
	const idNetField* field;
	for(i = 0, field = _playerStateFields ; i < _playerStateFieldCount ; i++, field++) 
	{
		fromF = (s32 *)((u8 *)from + field->offset);
		toF = (s32 *)((u8 *)to + field->offset);
		if(*fromF != *toF) 
		{
			lc = i+1;
		}
	}

	WriteByte(lc);	// # of changes

	for(i = 0, field = _playerStateFields ; i < lc ; i++, field++) 
	{
		fromF = (s32 *)((u8 *)from + field->offset);
		toF = (s32 *)((u8 *)to + field->offset);

		if(*fromF == *toF) 
		{
			WriteBits(0, 1);
			continue;
		}

		WriteBits(1, 1);
		WriteField(*toF, field->bits);
	}

	//
	// send the arrays
	//
	s32 statsbits = 0;
	for(i=0 ; i<ID_MAX_PS_STATS ; i++) 
	{
		if(to->stats[i] != from->stats[i])
		{
			statsbits |= 1<<i;
		}
	}
	s32 persistantbits = 0;
	for(i=0 ; i<ID_MAX_PS_PERSISTANT ; i++) 
	{
		if(to->persistant[i] != from->persistant[i])
		{
			persistantbits |= 1<<i;
		}
	}
	s32 ammobits = 0;
	for(i=0 ; i<ID_MAX_PS_WEAPONS ; i++)
	{
		if(to->ammo[i] != from->ammo[i]) 
		{
			ammobits |= 1<<i;
		}
	}
	s32 powerupbits = 0;
	for(i=0 ; i<ID_MAX_PS_POWERUPS ; i++) 
	{
		if(to->powerups[i] != from->powerups[i]) 
		{
			powerupbits |= 1<<i;
		}
	}

	if(!statsbits && !persistantbits && !ammobits && !powerupbits) 
	{
		WriteBits(0, 1);	// no change
		return ValidState();
	}
	WriteBits(1, 1);	// changed

	if(statsbits) 
	{
		WriteBits(1, 1);	// changed
		WriteBits(statsbits, ID_MAX_PS_STATS);
		for(i=0 ; i<ID_MAX_PS_STATS ; i++)
		{
			if(statsbits & (1<<i))
			{
				WriteShort(to->stats[i]);
			}
		}
	} 
	else 
	{
		WriteBits(0, 1);	// no change
	}

	if(persistantbits)
	{
		WriteBits(1, 1);	// changed
		WriteBits(persistantbits, ID_MAX_PS_PERSISTANT);
		for(i=0 ; i<ID_MAX_PS_PERSISTANT ; i++)
		{
			if(persistantbits & (1<<i))
			{
				WriteShort(to->persistant[i]);
			}
		}		
	} 
	else 
	{
		WriteBits(0, 1);	// no change
	}

	if(ammobits) 
	{
		WriteBits(1, 1);	// changed
		WriteBits(ammobits, ID_MAX_PS_WEAPONS);
		for(i=0 ; i<ID_MAX_PS_WEAPONS ; i++)
		{
			if(ammobits & (1<<i))
			{
				WriteShort(to->ammo[i]);
			}
		}	
	} 
	else 
	{
		WriteBits(0, 1);	// no change
	}

	if(powerupbits) 
	{
		WriteBits(1, 1);	// changed
		WriteBits(powerupbits, ID_MAX_PS_POWERUPS);
		for(i=0 ; i<ID_MAX_PS_POWERUPS ; i++)
		{
			if(powerupbits & (1<<i))
			{
				WriteLong(to->powerups[i]);
			}
		}
	} 
	else 
	{
		WriteBits(0, 1);	// no change
	}

	return ValidState();
}

void udtMessage::ReadDeltaPlayerDM3(idPlayerStateBase* to)
{
	const idNetField* field = _playerStateFields;
	const s32 fieldCount = _playerStateFieldCount;
	for(s32 i = 0; i < fieldCount; i++, field++)
	{
		if(!ReadBit())
		{
			continue;
		}

		s32* const toF = (s32*)((u8*)to + field->offset);
		*toF = ReadField(field->bits);
	}

	// Stats array.
	if(ReadBit())
	{
		const s32 mask = ReadShort();
		for(s32 i = 0; i < ID_MAX_PS_STATS; ++i)
		{
			if((mask & (1 << i)) != 0) // bit set?
			{
				// Stats can be negative.
				to->stats[i] = ReadSignedShort();
			}
		}
	}

	// Persistent array.
	if(ReadBit())
	{
		const s32 mask = ReadShort();
		for(s32 i = 0; i < ID_MAX_PS_PERSISTANT; ++i)
		{
			if((mask & (1 << i)) != 0) // bit set?
			{
				// @TODO: Can those be negative too?
				to->persistant[i] = ReadShort();
			}
		}
	}

	// Ammo array.
	if(ReadBit())
	{
		const s32 mask = ReadShort();
		for(s32 i = 0; i < ID_MAX_PS_WEAPONS; ++i)
		{
			if((mask & (1 << i)) != 0) // bit set?
			{
				to->ammo[i] = ReadShort();
			}
		}
	}

	// Power-ups array.
	if(ReadBit())
	{
		const s32 mask = ReadShort();
		for(s32 i = 0; i < ID_MAX_PS_POWERUPS; ++i)
		{
			if((mask & (1 << i)) != 0) // bit set?
			{
				// Yep, we read 32 bits.
				to->powerups[i] = ReadLong();
			}
		}
	}
}

bool udtMessage::RealReadDeltaPlayer(const idPlayerStateBase* from, idPlayerStateBase* to)
{
	s32			i, lc;
	s32			bits;
	s32			*fromF, *toF;
	idLargestPlayerState dummy;

	if(!from) 
	{
		from = &dummy;
		memset(&dummy, 0, sizeof(dummy));
	}
	memcpy(to, from, _protocolSizeOfPlayerState);
	
	if(_protocol <= udtProtocol::Dm48)
	{
		ReadDeltaPlayerDM3(to);
		return ValidState();
	}

	lc = ReadByte();
	if(lc > _playerStateFieldCount || lc < 0)
	{
		Context->LogError("udtMessage::RealReadDeltaPlayer: Invalid playerState field count: %d (max is %d) (in file: %s)", lc, _playerStateFieldCount, GetFileNamePtr());
		SetValid(false);
		return false;
	}

	const idNetField* field;
	for(i = 0, field = _playerStateFields; i < lc; i++, field++)
	{
		fromF = (s32 *)((u8 *)from + field->offset);
		toF = (s32 *)((u8 *)to + field->offset);

		if(ReadBit() == 0) 
		{
			*toF = *fromF;
			continue;
		} 

		*toF = ReadField(field->bits);
	}
	for(i = lc, field = &_playerStateFields[lc]; i < _playerStateFieldCount; i++, field++)
	{
		fromF = (s32 *)((u8 *)from + field->offset);
		toF = (s32 *)((u8 *)to + field->offset);
		*toF = *fromF;
	}

	// read the arrays
	if(ReadBit()) 
	{
		// parse stats
		if(ReadBit()) 
		{
			bits = ReadBits(ID_MAX_PS_STATS);
			for(i=0 ; i<ID_MAX_PS_STATS ; i++) 
			{
				if(bits & (1<<i))
				{
					to->stats[i] = ReadSignedShort();
				}
			}
		}

		// parse persistant stats
		if(ReadBit()) 
		{
			bits = ReadBits(ID_MAX_PS_PERSISTANT);
			for(i=0 ; i<ID_MAX_PS_PERSISTANT ; i++) 
			{
				if(bits & (1<<i)) 
				{
					to->persistant[i] = ReadShort();
				}
			}
		}

		// parse ammo
		if(ReadBit()) 
		{
			bits = ReadBits(ID_MAX_PS_WEAPONS);
			for(i=0 ; i<ID_MAX_PS_WEAPONS ; i++) 
			{
				if(bits & (1<<i)) 
				{
					to->ammo[i] = ReadShort();
				}
			}
		}

		// parse powerups
		if(ReadBit()) 
		{
			bits = ReadBits(ID_MAX_PS_POWERUPS);
			for(i=0 ; i<ID_MAX_PS_POWERUPS ; i++) 
			{
				if(bits & (1<<i)) 
				{
					to->powerups[i] = ReadLong();
				}
			}
		}
	}

	return ValidState();
}

/*
Writes part of a packet entities message, including the entity number.
Can delta from either a baseline or a previous packet_entity
If to is NULL, a remove entity update will be sent
If force is not set, then nothing at all will be generated if the entity is
identical, under the assumption that the in-order delta code will catch it.
*/
bool udtMessage::RealWriteDeltaEntity(const idEntityStateBase* from, const idEntityStateBase* to, bool force)
{
	s32			i, lc;
	s32			*fromF, *toF;

	idLargestEntityState dummy;
	if(from == NULL)
	{
		from = &dummy;
		Com_Memset (&dummy, 0, sizeof(dummy));
	}

	// all fields should be 32 bits to avoid any compiler packing issues
	// the "number" field is not part of the field list
	// if this assert fails, someone added a field to the entityState_t
	// struct without updating the message fields
#define STATIC_ASSERT(x) static_assert(x, #x)
	STATIC_ASSERT(EntityStateFieldCount68 + 1 == sizeof(idEntityState68) / 4);
	STATIC_ASSERT(EntityStateFieldCount73 + 1 == sizeof(idEntityState73) / 4);
	STATIC_ASSERT(EntityStateFieldCount90 + 1 == sizeof(idEntityState90) / 4);
	STATIC_ASSERT(EntityStateFieldCount91 + 1 == sizeof(idEntityState91) / 4);
	STATIC_ASSERT(sizeof(idEntityState3)  % 4 == 0);
	STATIC_ASSERT(sizeof(idEntityState48) % 4 == 0);
	STATIC_ASSERT(sizeof(idEntityState66) % 4 == 0);
	STATIC_ASSERT(sizeof(idEntityState67) % 4 == 0);
	STATIC_ASSERT(sizeof(idEntityState68) % 4 == 0);
	STATIC_ASSERT(sizeof(idEntityState73) % 4 == 0);
	STATIC_ASSERT(sizeof(idEntityState90) % 4 == 0);
	STATIC_ASSERT(sizeof(idEntityState91) % 4 == 0);
#undef STATIC_ASSERT

	// a NULL to is a delta remove message
	if(to == NULL) 
	{
		if(from == NULL) 
		{
			return ValidState();
		}
		WriteBits(from->number, GENTITYNUM_BITS);
		WriteBits(1, 1);
		return ValidState();
	}

	if(to->number < 0 || to->number >= MAX_GENTITIES) 
	{
		Context->LogError("udtMessage::RealWriteDeltaEntity: Bad entity number: %d (max is %d) (in file: %s)", to->number, MAX_GENTITIES - 1, GetFileNamePtr());
		SetValid(false);
		return false;
	}

	lc = 0;
	const idNetField* field;
	// build the change vector as bytes so it is endian independent
	for(i = 0, field = _entityStateFields ; i < _entityStateFieldCount ; i++, field++) 
	{
		fromF = (s32 *)((u8 *)from + field->offset);
		toF = (s32 *)((u8 *)to + field->offset);
		if(*fromF != *toF) 
		{
			lc = i+1;
		}
	}

	if(lc == 0) 
	{
		// nothing at all changed
		if(!force) 
		{
			return ValidState(); // nothing at all
		}
		// write two bits for no change
		WriteBits(to->number, GENTITYNUM_BITS);
		WriteBits(0, 1);		// not removed
		WriteBits(0, 1);		// no delta
		return ValidState();
	}

	WriteBits(to->number, GENTITYNUM_BITS);
	WriteBits(0, 1);			// not removed
	WriteBits(1, 1);			// we have a delta

	WriteByte(lc);	// # of changes

	for(i = 0, field = _entityStateFields ; i < lc ; i++, field++) 
	{
		fromF = (s32 *)((u8 *)from + field->offset);
		toF = (s32 *)((u8 *)to + field->offset);

		if(*fromF == *toF) 
		{
			WriteBits(0, 1);
			continue;
		}

		WriteBits(1, 1);
		if(*toF == 0)
		{
			WriteBits(0, 1);
			continue;
		}

		WriteBits(1, 1);
		WriteField(*toF, field->bits);
	}

	return ValidState();
}

/*
The entity number has already been read from the message, which
is how the from state is identified.

If the delta removes the entity, entityState_t->number will be set to MAX_GENTITIES-1

Can go from either a baseline or a previous packet_entity
*/

// @NOTE: Same values for dm3 and dm_48 confirmed.
static const u8 KnownBitMasks[32][7] =
{
	{ 0x60, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00 },
	{ 0x60, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
	{ 0x60, 0xC0, 0x00, 0x00, 0x00, 0x00, 0x00 },
	{ 0xE1, 0x00, 0x00, 0x00, 0x00, 0x20, 0x00 },
	{ 0x60, 0x80, 0x00, 0x00, 0x00, 0x10, 0x00 },
	{ 0xE0, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00 },
	{ 0xE0, 0xC0, 0x00, 0x00, 0x00, 0x00, 0x00 },
	{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x00 },
	{ 0x40, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00 },
	{ 0x20, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00 },
	{ 0x60, 0x80, 0x00, 0x00, 0x01, 0x00, 0x00 },
	{ 0xED, 0x07, 0x00, 0x00, 0x00, 0x80, 0x00 },
	{ 0xE0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
	{ 0xED, 0x07, 0x00, 0x00, 0x00, 0x30, 0x00 },
	{ 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
	{ 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
	{ 0xE0, 0xC0, 0x00, 0x00, 0x00, 0x10, 0x00 },
	{ 0x60, 0x00, 0x00, 0x00, 0x00, 0x10, 0x00 },
	{ 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
	{ 0xE1, 0x00, 0x00, 0x00, 0x04, 0x20, 0x00 },
	{ 0xE1, 0x00, 0xC0, 0x01, 0x20, 0x20, 0x00 },
	{ 0xE0, 0xC0, 0x00, 0x00, 0x01, 0x00, 0x00 },
	{ 0x60, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00 },
	{ 0x40, 0xC0, 0x00, 0x00, 0x00, 0x00, 0x00 },
	{ 0x60, 0xC0, 0x00, 0x00, 0x01, 0x00, 0x00 },
	{ 0x60, 0xC0, 0x00, 0x00, 0x00, 0x10, 0x00 },
	{ 0x60, 0x80, 0x00, 0x00, 0x01, 0x00, 0x01 },
	{ 0x60, 0x80, 0x00, 0x00, 0x00, 0x30, 0x00 },
	{ 0xE0, 0x80, 0x00, 0x00, 0x00, 0x10, 0x00 },
	{ 0x20, 0xC0, 0x00, 0x00, 0x00, 0x00, 0x00 },
	{ 0x60, 0x80, 0x00, 0x00, 0x00, 0x00, 0x02 },
	{ 0xE0, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00 }
};

void udtMessage::ReadDeltaEntityDM3(const idEntityStateBase* from, idEntityStateBase* to, s32 number)
{
	to->number = number;

	u8 bitMask[7]; // 50-51 bits used only.
	const s32 maskIndex = ReadBits(5);
	if(maskIndex == 0x1F)
	{
		for(s32 i = 0; i < 6; ++i)
		{
			bitMask[i] = (u8)ReadBits(8);
		}
		bitMask[6] = (u8)ReadBits(_protocol == udtProtocol::Dm3 ? 2 : 3);
	}
	else
	{
		// Let's not use memcpy for 7 bytes...
		for(s32 i = 0; i < 7; ++i)
		{
			bitMask[i] = KnownBitMasks[maskIndex][i];
		}
	}

	const idNetField* field = _entityStateFields;
	const s32 fieldCount = _entityStateFieldCount;
	for(s32 i = 0; i < fieldCount; i++, field++)
	{
		const s32* const fromF = (s32*)((u8*)from + field->offset);
		s32* const toF = (s32*)((u8*)to + field->offset);

		const s32 byteIndex = i >> 3;
		const s32 bitIndex = i & 7;
		if((bitMask[byteIndex] & (1 << bitIndex)) == 0)
		{
			*toF = *fromF;
			continue;
		}

		*toF = ReadField(field->bits);
	}
}

bool udtMessage::RealReadDeltaEntity(bool& addedOrChanged, const idEntityStateBase* from, idEntityStateBase* to, s32 number)
{
	if(number < 0 || number >= MAX_GENTITIES) 
	{
		Context->LogError("udtMessage::RealReadDeltaEntity: Bad delta entity number: %d (max is %d) (in file: %s)", number, MAX_GENTITIES - 1, GetFileNamePtr());
		SetValid(false);
		return false;
	}

	// check for a remove
	if(ReadBit() == 1) 
	{
		Com_Memset(to, 0, _protocolSizeOfEntityState);
		to->number = MAX_GENTITIES - 1;
		addedOrChanged = false;
		return ValidState();
	}

	// check for no delta
	if(ReadBit() == 0) 
	{
		Com_Memcpy(to, from, _protocolSizeOfEntityState);
		to->number = number;
		addedOrChanged = false;
		return ValidState();
	}

	addedOrChanged = true;
	if(_protocol <= udtProtocol::Dm48)
	{
		ReadDeltaEntityDM3(from, to, number);
		return ValidState();
	}
	
	const s32 fieldCount = ReadByte();
	const s32 maxFieldCount = _entityStateFieldCount;
	if(fieldCount > maxFieldCount || fieldCount < 0)
	{
		Context->LogError("udtMessage::RealReadDeltaEntity: Invalid entityState field count: %d (max is %d) (in file: %s)", fieldCount, maxFieldCount, GetFileNamePtr());
		SetValid(false);
		return false;
	}

	to->number = number;

	const idNetField* field = _entityStateFields;
	for(s32 i = 0; i < fieldCount; i++, field++) 
	{
		const s32* const fromF = (const s32*)((const u8*)from + field->offset);
		s32* const toF = (s32*)((u8*)to + field->offset);

		if(ReadBit() == 0) 
		{
			*toF = *fromF;
			continue;
		} 

		if(ReadBit() == 0)
		{
			*toF = 0;
			continue;
		}

		*toF = ReadField(field->bits);
	}

	field = &_entityStateFields[fieldCount];
	for(s32 i = fieldCount; i < maxFieldCount; i++, field++)
	{
		const s32* const fromF = (const s32*)((const u8*)from + field->offset);
		s32* const toF = (s32*)((u8*)to + field->offset);
		*toF = *fromF;
	}

	return ValidState();
}

void udtMessage::SetValid(bool valid)
{
	Buffer.valid = valid;
	if(valid)
	{
		_readBits = &udtMessage::RealReadBits;
		_readBit = Buffer.oob ? &udtMessage::RealReadBitNoHuffman : &udtMessage::RealReadBitHuffman;
		_readFloat = &udtMessage::RealReadFloat;
		_readString = &udtMessage::RealReadString;
		_readData = &udtMessage::RealReadData;
		_peekByte = &udtMessage::RealPeekByte;
		_readDeltaEntity = &udtMessage::RealReadDeltaEntity;
		_readDeltaPlayer = &udtMessage::RealReadDeltaPlayer;
		_writeBits = &udtMessage::RealWriteBits;
		_writeFloat = &udtMessage::RealWriteFloat;
		_writeString = &udtMessage::RealWriteString;
		_writeDeltaPlayer = &udtMessage::RealWriteDeltaPlayer;
		_writeDeltaEntity = &udtMessage::RealWriteDeltaEntity;
	}
	else
	{
		_readBits = &udtMessage::DummyReadCount;
		_readBit = &udtMessage::DummyRead;
		_readFloat = &udtMessage::DummyRead;
		_readString = &udtMessage::DummyReadString;
		_readData = &udtMessage::DummyReadData;
		_peekByte = &udtMessage::DummyRead;
		_readDeltaEntity = &udtMessage::DummyReadDeltaEntity;
		_readDeltaPlayer = &udtMessage::DummyReadDeltaPlayer;
		_writeBits = &udtMessage::DummyWriteCount;
		_writeFloat = &udtMessage::DummyWrite;
		_writeString = &udtMessage::DummyWriteString;
		_writeDeltaPlayer = &udtMessage::DummyWriteDeltaPlayer;
		_writeDeltaEntity = &udtMessage::DummyWriteDeltaEntity;
	}
}
