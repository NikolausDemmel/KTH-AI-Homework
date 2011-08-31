
import sys

def print_pw(pw):
    print ''.join(pw)

def main():
    l = int(sys.argv[1])
    stack = [(0,[])]
    counter = 0
    while not len(stack) == 0:
        i,curr = stack.pop()
        for x in "ABCDEFGHI":
            # conditions for filtering invalid pws
            if i == 0 and not (x in "AH"):
                continue
            if i > 0 and ((x == 'A' and curr[-1] == 'C') or (x == 'C' and curr[-1] == 'A')):
                    continue
            if curr.count('A') == 3 and x == 'A':
                continue
            # add to working list or count
            next = curr[:]
            next.append(x)
            if i == l-1:
                counter += 1
            else:
                stack.append((i+1,next))
    print counter

if __name__ == '__main__':
    main()
