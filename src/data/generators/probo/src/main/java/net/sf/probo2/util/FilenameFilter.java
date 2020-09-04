package net.sf.probo2.util;

import java.io.File;

import org.apache.commons.io.filefilter.IOFileFilter;

/**
 * Simple file filter
 * @author Matthias Thimm
 */
public class FilenameFilter implements IOFileFilter {
	
	private String postfix;
	
	public FilenameFilter(String postfix){
		this.postfix = postfix;
	}	
	@Override
	public boolean accept(File dir, String name) {
		return name.endsWith(this.postfix);
	}
	@Override
	public boolean accept(File arg0) {			
		return arg0.getName().endsWith(this.postfix);
	}		
}
