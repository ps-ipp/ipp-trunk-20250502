
/* I need a structure which will be appropriate to carry the psMetadataConfig data.  the main 
   requirements are:

   - name-based components
   - string-indexing on columns

   basic elements:

   container->item->element->value

   element: name, value (type?)
   item->name, Nelement, elements
   container->items, Nitems

   user interactions

   blob list
   blob listitems (blob)
   blob create (blob)
   blob delete (blob)
   blob listkeys (blob.item)

   blob getvalue (blob.item.key) -var word
   blob setvalue (blob.item.key) value

   blob getitem (blob.item)  : list all key/value pairs
   blob newitem (blob.item)
   blob delitem (blob.item)
   blob popitem (blob)

   blob readqueue (queue)    : convert queue in MDC format to blob
   *** this needs to be able to match by keys against existing items
   
   need equivalents to:
   queuepush -uniq key
   queuepop -
   queuesize
   

   items should be sorted by name so we can lookup an item quickly



   other related opihi data concepts
   
   $a = @function (output of function set to value?)

*/
