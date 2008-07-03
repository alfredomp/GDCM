/*=========================================================================

  Program: GDCM (Grass Root DICOM). A DICOM library
  Module:  $URL$

  Copyright (c) 2006-2008 Mathieu Malaterre
  All rights reserved.
  See Copyright.txt or http://gdcm.sourceforge.net/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*
 * TODO:
 * Should implement the gdcmiodvfy here
 * I need to implement gdcmoverlay here (print info on overlay / img / LUT ...)
 */
#include "gdcmReader.h"
#include "gdcmImageReader.h"
#include "gdcmMediaStorage.h"
#include "gdcmFile.h"
#include "gdcmDataSet.h"
#include "gdcmUIDs.h"
#include "gdcmGlobal.h"
#include "gdcmModules.h"
#include "gdcmDefs.h"
#include "gdcmOrientation.h"
#include "gdcmVersion.h"

#include "puff.h"

#include <iostream>

#include <stdio.h>     /* for printf */
#include <stdint.h>
#include <stdlib.h>    /* for exit */
#include <string.h>
#include <getopt.h>
#include <sys/types.h>
#include <sys/stat.h>


int checkmagick(unsigned char *input)
{
  if( input[128+0] == 'D' 
   && input[128+1] == 'I' 
   && input[128+2] == 'C' 
   && input[128+3] == 'M' )
    {
    return 1;
    }
  return 0;
}

//int checkdeflate(int argc, char *argv[])
int checkdeflate(const char *name)
{
  int ret;
  unsigned char *source;
  unsigned long len, sourcelen, destlen;

  unsigned long size;
  unsigned long size1;
  unsigned char *buf;
  FILE *in;
  struct stat s;
  //const char *name = 0;
  union { uint32_t tag; uint16_t tags[2]; char bytes[4]; } tag;
  char vr[3];
  uint16_t vl;
  uint32_t value;

  //if (argc < 2) return 2;
  //name = argv[1];

  len = 0;
  if (stat(name, &s)) 
  {
    fprintf( stderr, "Cannot stat: %s\n", name );
	  return 1;
  }
  if ((s.st_mode & S_IFMT) != S_IFREG) 
  {
    fprintf( stderr, "not a regular file\n" );
	  return 1;
  }
  size = (unsigned long)(s.st_size);
  if (size == 0 || (off_t)size != s.st_size) 
  {
    fprintf( stderr, "size mismatch\n" );
	  return 1;
  }
  in = fopen(name, "r");
  if (in == NULL) 
  {
    fprintf( stderr, "in is NULL\n" );
	  return 1;
  }
  buf = (unsigned char*)malloc(size);
  if (buf != NULL && (size1 = fread(buf, 1, size, in)) != size) {
    free(buf);
    buf = NULL;
    fprintf( stderr, "could not fread: %u bytes != %u\n", size, size1 );
    fprintf( stderr, "feof: %i ferror %i\n", feof(in), ferror(in) );
  }
  fclose(in);
  len = size;
  source = buf;
  if( source == NULL ) {
    fprintf( stderr, "source is NULL\n" );
	  return 1;
  }
  sourcelen = len;

  if( !checkmagick(source) )
    {
    fprintf( stderr, "checkmagick failed\n" );
    return 1;
    }
  // magick succeed so skip header:
  source += 128 + 4;
  sourcelen -= 128 + 4;

  memcpy(&tag, source, sizeof(tag) );
  fprintf( stdout,"tag: %d, %d\n", tag.tags[0], tag.tags[1] );
  source += sizeof(tag);
  sourcelen -= sizeof(tag);

  vr[2] = 0;
  memcpy(vr, source, 2);
  printf( "vr: %s\n", vr);

  source += 2;
  sourcelen -= 2;

  memcpy(&vl, source, sizeof(vl));
  printf( "vl: %d\n", vl);

  source += sizeof(vl);
  sourcelen -= sizeof(vl);

  memcpy(&value, source, sizeof(value));
  printf( "value: %d\n", value);

  source += sizeof(value);
  sourcelen -= sizeof(value);

  source += value;
  sourcelen -= value;

  len = sourcelen;
  if( len % 2 )
    {
    printf( "len of bit stream is odd: %d. Continuing anyway\n", len );
    }
  else
    {
    printf( "deflate stream has proper length: %d\n", len );
    }

  ret = puff(NULL, &destlen, source, &sourcelen);

  if (ret)
    fprintf(stdout,"puff() failed with return code %d\n", ret);
  else {
    fprintf(stdout,"puff() succeeded uncompressing %lu bytes\n", destlen);
    if (sourcelen < len) printf("%lu compressed bytes unused\n",
      len - sourcelen);
  }
  free(buf);
  return ret;
}

void PrintVersion()
{
  std::cout << "gdcminfo: gdcm " << gdcm::Version::GetVersion() << " ";
  const char date[] = "$Date: 2008-06-02 09:20:26 +0200 (Mon, 02 Jun 2008) $";
  std::cout << date << std::endl;
}

void PrintHelp()
{
  PrintVersion();
  std::cout << "Usage: gdcminfo [OPTION]... [FILE]..." << std::endl;
  std::cout << "by default gdcmdump, only dumps a DICOM file, that is the minimal operations required to\n"
   " display information reader need to see the structure of a DICOM file and some value in its fields" << std::endl;
  std::cout << "Parameter:" << std::endl;
  std::cout << "  -i --input     DICOM filename or directory" << std::endl;
  std::cout << "Options:" << std::endl;
  std::cout << "  -d --check-deflate   check if file is proper deflate syntax." << std::endl;
//  std::cout << "  -b --check-big-endian   check if file is ." << std::endl;
  std::cout << "  -V --verbose   more verbose (warning+error)." << std::endl;
  std::cout << "  -W --warning   print warning info." << std::endl;
  std::cout << "  -D --debug     print debug info." << std::endl;
  std::cout << "  -E --error     print error info." << std::endl;
  std::cout << "  -h --help      print help." << std::endl;
  std::cout << "  -v --version   print version." << std::endl;
}


int main(int argc, char *argv[])
{
  int c;
  std::string filename;
  int deflate = 0; // check deflate
  int verbose = 0;
  int warning = 0;
  int help = 0;
  int recursive = 0;
  int version = 0;
  int debug = 0;
  int error = 0;
  while (1) {
    int option_index = 0;
    static struct option long_options[] = {
        {"input", 1, 0, 0},
        {"check-deflate", 0, &deflate, 1},
        {"verbose", 0, &verbose, 1},
        {"warning", 0, &warning, 1},
        {"debug", 0, &debug, 1},
        {"error", 0, &error, 1},
        {"help", 0, &help, 1},
        {"version", 0, &version, 1},
        {0, 0, 0, 0} // required
    };
    static const char short_options[] = "i:VWDEhv";
    c = getopt_long (argc, argv, short_options,
      long_options, &option_index);
    if (c == -1)
      {
      break;
      }

    switch (c)
      {
    case 0:
    case '-':
        {
        const char *s = long_options[option_index].name;
        //printf ("option %s", s);
        if (optarg)
          {
          if( option_index == 0 ) /* input */
            {
            assert( strcmp(s, "input") == 0 );
            assert( filename.empty() );
            filename = optarg;
            }
          //printf (" with arg %s", optarg);
          }
        //printf ("\n");
        }
      break;

    case 'i':
      //printf ("option i with value '%s'\n", optarg);
      assert( filename.empty() );
      filename = optarg;
      break;

   case 'V':
      verbose = 1;
      break;

    case 'W':
      warning = 1;
      break;

    case 'D':
      debug = 1;
      break;

    case 'E':
      error = 1;
      break;

    case 'h':
      help = 1;
      break;

    case 'v':
      version = 1;
      break;

    case '?':
      break;

    default:
      printf ("?? getopt returned character code 0%o ??\n", c);
      }
  }

  if (optind < argc)
    {
    //printf ("non-option ARGV-elements: %d", optind );
    //while (optind < argc)
    //  {
    //  printf ("%s\n", argv[optind++]);
    //  }
    //printf ("\n");
    // Ok there is only one arg, easy, it's the filename:
    int v = argc - optind;
    if( v == 1 )
      {
      filename = argv[optind];
      }
    }

  if( version )
    {
    //std::cout << "version" << std::endl;
    PrintVersion();
    return 0;
    }

  if( help )
    {
    //std::cout << "help" << std::endl;
    PrintHelp();
    return 0;
    }

  if( filename.empty() )
    {
    //std::cerr << "Need input file (-i)\n";
    PrintHelp();
    return 1;
    }
  
  // Debug is a little too verbose
  gdcm::Trace::SetDebug( debug );
  gdcm::Trace::SetWarning( warning );
  gdcm::Trace::SetError( error );
  // when verbose is true, make sure warning+error are turned on:
  if( verbose )
    {
    gdcm::Trace::SetWarning( verbose );
    gdcm::Trace::SetError( verbose);
    }

  if( deflate )
  {
    return checkdeflate(filename.c_str());
  }
 
  //const char *filename = argv[1];
  //std::cout << "filename: " << filename << std::endl;
  gdcm::Reader reader;
  reader.SetFileName( filename.c_str() );
  if( !reader.Read() )
    {
    std::cerr << "Failed to read: " << filename << std::endl;
    return 1;
    }
  const gdcm::File &file = reader.GetFile();
  const gdcm::DataSet &ds = file.GetDataSet();
  gdcm::MediaStorage ms;
  ms.SetFromFile(file);

  gdcm::UIDs uid;
  uid.SetFromUID( ms.GetString() );
  std::cout << "MediaStorage is " << ms << " [" << uid.GetName() << "]" << std::endl;

  if( gdcm::MediaStorage::IsImage( ms ) )
    {
    gdcm::ImageReader reader;
    reader.SetFileName( filename.c_str() );
    if( !reader.Read() )
      {
      return 1;
      }
    const gdcm::File &file = reader.GetFile();
    const gdcm::DataSet &ds = file.GetDataSet();
    const gdcm::Image &image = reader.GetImage();
    const double *dircos = image.GetDirectionCosines();
    gdcm::Orientation::OrientationType type = gdcm::Orientation::GetType(dircos);
    const char *label = gdcm::Orientation::GetLabel( type );
    image.Print( std::cout );
    std::cout << "Orientation Label: " << label << std::endl;

    }

  if( ms == gdcm::MediaStorage::MRImageStorage && false )
    {
    const gdcm::Global& g = gdcm::Global::GetInstance();
    const gdcm::Defs &defs = g.GetDefs();
    const gdcm::Modules &modules = defs.GetModules();
    const gdcm::Module module = modules.GetModule( "MR Image Module Attributes" );
    //std::cout << module << std::endl;
    gdcm::Module::ConstIterator it = module.Begin();
    for(; it != module.End(); ++it)
      {
      const gdcm::Tag &tag = it->first;
      const gdcm::ModuleEntry &me = it->second;
      const gdcm::Type &type = me.GetType();
      if( ds.FindDataElement( tag ) )
        {
        // element found
        const gdcm::DataElement &de = ds.GetDataElement( tag );
        if ( de.IsEmpty() && (type == gdcm::Type::T1 || type == gdcm::Type::T1C ) )
          {
          std::cerr << "T1 element cannot be empty: " << de << std::endl;
          }
        }
      else
        {
        //if( me.GetType() == )
          {
          std::cerr << "DataSet is missing tag: " << tag << std::endl;
          std::cerr << "ModuleEntry specify: " << me << std::endl;
          }
        }
      }
    }
    {
    const gdcm::Global& g = gdcm::Global::GetInstance();
    const gdcm::Defs &defs = g.GetDefs();
    const gdcm::IODs &iods = defs.GetIODs();
    const gdcm::IOD &iod = iods.GetIOD( "MR Image IOD Modules" );
    std::cout << iod << std::endl;
    }


  return 0;
}
