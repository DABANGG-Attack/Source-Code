import Levenshtein as lev
from lev import distance
import sys
with open(sys.argv[1], 'r') as file:
   sender = file.read() 
with open(sys.argv[2], 'r') as file:
   receiver = file.read()
error = distance(sender,receiver)
print(len(sender))
print(len(receiver))
maxlen = max(len(sender),len(receiver))
print( 'Maxlen: ' , maxlen )
accuracy = ((maxlen-error)*100.0)/maxlen
print( 'Error: ' , error)
print( 'Accuracy: ', accuracy)