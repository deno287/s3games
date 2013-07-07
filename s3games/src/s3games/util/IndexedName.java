/*
 * To change this template, choose Tools | Templates
 * and open the template in the editor.
 */

package s3games.util;

/**
 *
 * @author petrovic16
 */
public class IndexedName
{
    public String fullName;
    public String baseName;
    public Integer[] index;

    public IndexedName(String name)
    {
        fullName = name;
        if (name.indexOf('(') < 0)
        {
            baseName = name;
            index = new Integer[0];
        }
        else
        {
            String[] nm = name.split("(", 2);
            baseName = nm[0];
            String[] args = nm[1].substring(1, nm[1].length() - 1).split(",");
            index = new Integer[args.length];
            for (int i = 0; i < index.length; i++)
                index[i] = Integer.parseInt(args[i].trim());
        }

    }

}